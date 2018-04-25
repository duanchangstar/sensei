#include "gtest/gtest.h"
#include "hardware_frontend/serial_frontend_internal.h"

using namespace sensei;
using namespace serial_frontend;

static uint8_t test_msg[] = { 0x1, 0x2, 0x3, 0xff, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x8c, 0x3, 0x0, 0x0, 0x64, 0x1, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
                              0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xe8, 
                              0xe2, 0xf6, 0x10, 0xc3, 0x4, 0x4, 0x5, 0x6 };

// Test standalone functions
TEST (TestHelperFunctions, test_compare_packet_header)
{
    PACKET_HEADER hdr1 = {1, 2, 3};
    PACKET_HEADER hdr2 = {4, 5, 6};
    EXPECT_EQ(0, compare_packet_header(hdr1, hdr1));
    EXPECT_NE(0, compare_packet_header(hdr1, hdr2));
}

TEST (TestHelperFunctions, test_calculate_crc)
{
    uint16_t crc = calculate_crc(reinterpret_cast<sSenseiDataPacket*>(test_msg));
    EXPECT_EQ(0x04c3, crc);
}

/*
 * Test values from http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
 */
TEST (TestHelperFunctions, test_quat_to_euler)
{
    // Test general case
    EulerAngles angles = quat_to_euler(1, 0.3, 0.2, 0.4);
    EXPECT_FLOAT_EQ(0.260602392, angles.yaw);
    EXPECT_FLOAT_EQ(1.168080485, angles.pitch);
    EXPECT_FLOAT_EQ(0.721654851, angles.roll);

    // Test a singularity (North Pole)
    angles = quat_to_euler(0.5, 0.5, 0.5, 0.5);
    EXPECT_FLOAT_EQ(1.570796327, angles.yaw);
    EXPECT_FLOAT_EQ(1.570796327, angles.pitch);
    EXPECT_FLOAT_EQ(0, angles.roll);
}


TEST (TestMessageConcatenator, test_message_concatenation)
{
    MessageConcatenator module_under_test;
    sSenseiDataPacket packet_1;
    sSenseiDataPacket packet_2;
    packet_1.continuation = 1;
    memcpy(packet_1.payload, "This is the first part of a two part message and ", SENSEI_PAYLOAD_LENGTH);
    packet_2.continuation = 0;
    memcpy(packet_2.payload, "This is part 2 of a two part message", SENSEI_PAYLOAD_LENGTH);

    const char* assembled_msg = module_under_test.add(&packet_2);
    ASSERT_NE(nullptr, assembled_msg);
    EXPECT_STREQ("This is part 2 of a two part message", reinterpret_cast<const char*>(assembled_msg));
    assembled_msg = module_under_test.add(&packet_1);
    EXPECT_EQ(nullptr, assembled_msg);
    assembled_msg = module_under_test.add(&packet_2);
    ASSERT_NE(nullptr, assembled_msg);
    EXPECT_STREQ("This is the first part of a two part message and This is part 2 of a two part message", reinterpret_cast<const char*>(assembled_msg));
}