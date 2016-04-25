/**
 * @brief Creates serial commands from general set functions. Also caches values on
 * a per pin basis since the teensy set commands set all parameters at once.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SENSEI_SERIAL_COMMAND_CREATOR_H
#define SENSEI_SERIAL_COMMAND_CREATOR_H

#include <vector>
#include <message/command_defs.h>

#include "../../common/sensei_serial_protocol.h"
#include "serial_frontend_internal.h"
#include "message/base_message.h"

namespace sensei {
namespace serial_frontend {

struct pin_config
{
    uint8_t           pintype;
    sPinConfiguration cfg_data;
} __attribute__((packed)) ;

class SerialCommandCreator {
public:
    SerialCommandCreator(int max_pins);
    ~SerialCommandCreator();

    const sSenseiDataPacket* make_initialize_system_cmd(uint32_t timestamp, int ticks_delay, int pins, int digital_pins);
    const sSenseiDataPacket* make_set_digital_pin_cmd(int pin_id, uint32_t timestamp, bool value);
    const sSenseiDataPacket* make_set_bank_cmd(int pin_id, uint32_t timestamp, int value);
    const sSenseiDataPacket* make_set_sampling_rate_cmd(uint32_t timestamp, float sampling_rate);
    const sSenseiDataPacket* make_get_value_cmd(int pin_id, uint32_t timestamp);
    const sSenseiDataPacket* make_config_pintype_cmd(int pin_id, uint32_t timestamp, PinType type);
    const sSenseiDataPacket* make_config_enabled_cmd(uint32_t timestamp, bool enabled);
    const sSenseiDataPacket* make_config_sendingmode_cmd(int pin_id, uint32_t timestamp, SendingMode mode);
    const sSenseiDataPacket* make_config_delta_ticks_cmd(int pin_id, uint32_t timestamp, int ticks);
    const sSenseiDataPacket*make_config_adc_bitres_cmd(int pin_id, uint32_t timestamp, int bits);
    const sSenseiDataPacket* make_config_filter_order_cmd(int pin_id, uint32_t timestamp, int order);
    const sSenseiDataPacket* make_config_lowpass_cutoff_cmd(int pin_id, uint32_t timestamp, float cutoff);
    const sSenseiDataPacket* make_config_slidermode_cmd(int pin_id, uint32_t timestamp, int mode);
    const sSenseiDataPacket* make_config_slider_threshold_cmd(int pin_id, uint32_t timestamp, int threshold);

private:
    int _max_pins;
    sSenseiDataPacket _cmd_buffer;
    std::vector<pin_config> _cached_cfgs;
};

void initialize_common_data(sSenseiDataPacket& packet, uint32_t timestamp, uint8_t command);
void fill_data(const pin_config& cached_cfg, sSenseiDataPacket& packet, uint32_t timestamp, uint8_t command);

} // end namespace serial_receiver
} // end namespace sensei


#endif //SENSEI_SERIAL_COMMAND_CREATOR_H