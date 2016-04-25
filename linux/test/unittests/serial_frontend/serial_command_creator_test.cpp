
#include "gtest/gtest.h"

#define private public
#include "serial_frontend/serial_command_creator.cpp"


using namespace sensei;
using namespace serial_frontend;

static const uint32_t test_tstamp = test_tstamp;
static const int MAX_NUMBER_OFF_PINS = 64;

class TestSerialCommandCreator : public ::testing::Test
{
protected:
    TestSerialCommandCreator() :
            _module_under_test(MAX_NUMBER_OFF_PINS)
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
    SerialCommandCreator _module_under_test;
};

TEST_F(TestSerialCommandCreator, test_initialization)
{
    pin_config p = _module_under_test._cached_cfgs[MAX_NUMBER_OFF_PINS - 1];
    EXPECT_EQ(0, p.pintype);
    EXPECT_EQ(MAX_NUMBER_OFF_PINS -1, p.cfg_data.idxPin);
    EXPECT_EQ(0, p.cfg_data.sendingMode);
    EXPECT_EQ(0, p.cfg_data.deltaTicksContinuousMode);
    EXPECT_EQ(0, p.cfg_data.ADCBitResolution);
    EXPECT_FLOAT_EQ(0, p.cfg_data.lowPassCutOffFilter);
    EXPECT_EQ(0, p.cfg_data.sliderMode);
    EXPECT_EQ(0, p.cfg_data.sliderThreshold);
}

TEST_F(TestSerialCommandCreator, test_initialize_common_data)
{
    sSenseiDataPacket packet;
    initialize_common_data(packet, test_tstamp, CONFIGURE_PIN);
    EXPECT_EQ(0, compare_packet_header(START_HEADER, packet.start_header));
    EXPECT_EQ(0, compare_packet_header(STOP_HEADER, packet.stop_header));
    EXPECT_EQ((uint32_t)test_tstamp, packet.timestamp);
    EXPECT_EQ(CONFIGURE_PIN, packet.cmd);
}

TEST_F(TestSerialCommandCreator, test_make_initialize_system_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_initialize_system_cmd(test_tstamp, 1, 32, 16);
    const sSystemInitialization* data = reinterpret_cast<const sSystemInitialization*>(&packet->payload);
    EXPECT_EQ(INITIALIZE_SYSTEM, packet->cmd);
    EXPECT_EQ(1, data->ticksDelayRtTask);
    EXPECT_EQ(32, data->nPins);
    EXPECT_EQ(16, data->nDigitalPins);
    EXPECT_EQ(test_tstamp, packet->timestamp);
}


TEST_F(TestSerialCommandCreator, test_make_set_digital_pin_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_set_digital_pin_cmd(5, test_tstamp, 1);
    const teensy_set_value_cmd* data = reinterpret_cast<const teensy_set_value_cmd*>(&packet->payload);
    EXPECT_EQ(SET_PIN, packet->sub_cmd);
    EXPECT_EQ(SET_DIGITAL_PINS, packet->cmd);
    EXPECT_EQ(5, data->pin_idx);
    EXPECT_EQ(1, data->value);
    EXPECT_EQ(test_tstamp, packet->timestamp);
}

TEST_F(TestSerialCommandCreator, test_make_set_bank_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_set_bank_cmd(5, test_tstamp, 1);
    const teensy_set_value_cmd* data = reinterpret_cast<const teensy_set_value_cmd*>(&packet->payload);
    EXPECT_EQ(SET_BANK, packet->sub_cmd);
    EXPECT_EQ(SET_DIGITAL_PINS, packet->cmd);
    EXPECT_EQ(5, data->pin_idx);
    EXPECT_EQ(1, data->value);
    EXPECT_EQ(test_tstamp, packet->timestamp);
}

TEST_F(TestSerialCommandCreator, test_make_set_sampling_rate_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_set_sampling_rate_cmd(test_tstamp, 500);
    const teensy_set_samplerate_cmd* data = reinterpret_cast<const teensy_set_samplerate_cmd*>(&packet->payload);
    EXPECT_EQ(EMPTY, packet->sub_cmd);
    EXPECT_EQ(SET_SAMPLING_RATE, packet->cmd);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(2, data->sample_rate_divisor);

    packet = _module_under_test.make_set_sampling_rate_cmd(test_tstamp, 2);
    data = reinterpret_cast<const teensy_set_samplerate_cmd*>(&packet->payload);
    EXPECT_EQ(0, data->sample_rate_divisor);

    packet = _module_under_test.make_set_sampling_rate_cmd(test_tstamp, 333);
    data = reinterpret_cast<const teensy_set_samplerate_cmd*>(&packet->payload);
    EXPECT_EQ(3, data->sample_rate_divisor);
}

TEST_F(TestSerialCommandCreator, test_make_get_value_cmd)
{
    const sSenseiDataPacket *packet = _module_under_test.make_get_value_cmd(12, test_tstamp);
    const teensy_set_value_cmd* data = reinterpret_cast<const teensy_set_value_cmd*>(&packet->payload);
    EXPECT_EQ(EMPTY, packet->sub_cmd);
    EXPECT_EQ(GET_VALUE, packet->cmd);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(12, data->pin_idx);
}

TEST_F(TestSerialCommandCreator, test_make_config_pintype_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_pintype_cmd(3, test_tstamp, PinType::ANALOG_INPUT);
    EXPECT_EQ(PIN_ANALOG_INPUT, packet->sub_cmd);
    EXPECT_EQ(test_tstamp, packet->timestamp);
}

TEST_F(TestSerialCommandCreator, test_make_config_sendingmode_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_sendingmode_cmd(3, test_tstamp, SendingMode::ON_REQUEST);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(SENDING_MODE_ON_REQUEST, pin_config->sendingMode);
    EXPECT_EQ(3, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_delta_ticks_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_delta_ticks_cmd(4, test_tstamp, 10);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(10, pin_config->deltaTicksContinuousMode);
    EXPECT_EQ(4, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_adc_bitres_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_adc_bitres_cmd(5, test_tstamp, 10);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(PIN_ADC_RESOLUTION_10_BIT, pin_config->ADCBitResolution);
    EXPECT_EQ(5, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_filter_order_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_filter_order_cmd(6, test_tstamp, 4);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(4, pin_config->filterOrder);
    EXPECT_EQ(6, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_lowpass_cutoff_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_lowpass_cutoff_cmd(7, test_tstamp, 1.234);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_FLOAT_EQ(1.234, pin_config->lowPassCutOffFilter);
    EXPECT_EQ(7, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_slidermode_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_slidermode_cmd(8, test_tstamp, 1);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(1, pin_config->sliderMode);
    EXPECT_EQ(8, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_make_config_slider_threshold_cmd)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_slider_threshold_cmd(9, test_tstamp, 5);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(test_tstamp, packet->timestamp);
    EXPECT_EQ(5, pin_config->sliderThreshold);
    EXPECT_EQ(9, pin_config->idxPin);
}

TEST_F(TestSerialCommandCreator, test_cacheing)
{
    const sSenseiDataPacket* packet = _module_under_test.make_config_adc_bitres_cmd(10, test_tstamp, 10);
    packet = _module_under_test.make_config_pintype_cmd(10, test_tstamp, PinType::ANALOG_INPUT);
    packet = _module_under_test.make_config_sendingmode_cmd(10, 0x12341234, SendingMode::CONTINUOUS);
    const sPinConfiguration* pin_config = reinterpret_cast<const sPinConfiguration*>(&packet->payload);
    EXPECT_EQ(static_cast<uint32_t>(0x12341234), packet->timestamp);
    EXPECT_EQ(PIN_ADC_RESOLUTION_10_BIT, pin_config->ADCBitResolution);
    EXPECT_EQ(PIN_ANALOG_INPUT, packet->sub_cmd);
    EXPECT_EQ(SENDING_MODE_CONTINUOUS, pin_config->sendingMode);
    EXPECT_EQ(10, pin_config->idxPin);
}