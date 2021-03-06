/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SENSEI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SENSEI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SENSEI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Classes that perform transformation on received sensors data
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 *
 * These are instantiated internally as components of MappingProcessor
 */
#include <cassert>
#include <cmath>

#include "mapping/sensor_mappers.h"
#include "message/message_factory.h"
#include "utils.h"
#include "logging.h"

namespace {

static const int MAX_ADC_BIT_RESOLUTION = 16;
static const int DEFAULT_ADC_BIT_RESOLUTION = 12;
static const float DEFAULT_FILTER_TIME_CONSTANT = 0.020f; // 20 ms

static const float PREVIOUS_VALUE_THRESHOLD = 1.0e-4f;

}; // Anonymous namespace

SENSEI_GET_LOGGER_WITH_MODULE_NAME("mapper");

using namespace sensei;
using namespace sensei::mapping;


////////////////////////////////////////////////////////////////////////////////
// BaseSensorMapper
////////////////////////////////////////////////////////////////////////////////

BaseSensorMapper::BaseSensorMapper(SensorType type, int index) :
    _sensor_type(type),
    _sensor_index(index),
    _enabled(false),
    _multiplexed(false),
    _multiplexer_data{0,0},
    _sending_mode(SendingMode::OFF),
    _delta_ticks_sending(1),
    _previous_value(0.0f),
    _invert_value(false),
    _send_timestamp(false)
{}

CommandErrorCode BaseSensorMapper::apply_command(const Command *cmd)
{
    assert(cmd->index() == _sensor_index);

    CommandErrorCode status = CommandErrorCode::OK;

    // Handle here per-sensor configuration common between sensor types
    switch(cmd->type())
    {
    case CommandType::SET_ENABLED:
        {
            const auto typed_cmd = static_cast<const SetEnabledCommand*>(cmd);
            _enabled = typed_cmd->data();
        };
        break;

    case CommandType::SET_SENSOR_HW_TYPE:
        {
            const auto typed_cmd = static_cast<const SetSensorHwTypeCommand*>(cmd);
            _hw_type = typed_cmd->data();
        };
        break;

    case CommandType::SET_HW_PINS:
        {
            const auto typed_cmd = static_cast<const SetHwPinsCommand*>(cmd);
            _hw_pins = typed_cmd->data();
        };
        break;

    case CommandType::SET_SENDING_MODE:
        {
            const auto typed_cmd = static_cast<const SetSendingModeCommand*>(cmd);
            _sending_mode = typed_cmd->data();
        };
        break;

    case CommandType::SET_INVERT_ENABLED:
        {
            const auto typed_cmd = static_cast<const SetInvertEnabledCommand*>(cmd);
            _invert_value = typed_cmd->data();
        };
        break;

    case CommandType::SET_SENDING_DELTA_TICKS:
        {
            const auto typed_cmd = static_cast<const SetSendingDeltaTicksCommand*>(cmd);
            if (typed_cmd->data() > 0)
            {
                _delta_ticks_sending = typed_cmd->data();
            }
            else
            {
                _delta_ticks_sending = 1;
                status = CommandErrorCode::INVALID_VALUE;
            }
        };
        break;

    case CommandType::SET_MULTIPLEXED:
        {
            const auto typed_cmd = static_cast<const SetMultiplexedSensorCommand*>(cmd);
            _multiplexer_data = typed_cmd->data();
            _multiplexed = true;
        };
        break;

    case CommandType::SET_SEND_TIMESTAMP_ENABLED:
        {
            const auto typed_cmd = static_cast<const SetSendTimestampEnabledCommand*>(cmd);
            _send_timestamp = typed_cmd->data();
        }
        break;

    case CommandType::SET_FAST_MODE:
        {
            const auto typed_cmd = static_cast<const SetFastModeCommand*>(cmd);
            _fast_mode = typed_cmd->data();
        }
        break;

    default:
        status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
        break;

    }

    return status;

}

void BaseSensorMapper::put_config_commands_into(CommandIterator out_iterator)
{
    MessageFactory factory;
    *out_iterator = factory.make_set_sensor_type_command(_sensor_index, _sensor_type);
    *out_iterator = factory.make_set_sensor_hw_type_command(_sensor_index, _hw_type);
    *out_iterator = factory.make_set_hw_pins_command(_sensor_index, _hw_pins);
    *out_iterator = factory.make_set_enabled_command(_sensor_index, _enabled);
    *out_iterator = factory.make_set_sending_mode_command(_sensor_index, _sending_mode);
    *out_iterator = factory.make_set_sending_delta_ticks_command(_sensor_index, _delta_ticks_sending);
    *out_iterator = factory.make_set_invert_enabled_command(_sensor_index, _invert_value);
    *out_iterator = factory.make_set_send_timestamp_enabled(_sensor_index, _send_timestamp);
    *out_iterator = factory.make_set_fast_mode_command(_sensor_index, _fast_mode);
    if (_multiplexed)
    {
        *out_iterator = factory.make_set_multiplexed_sensor_command(_sensor_index,
                                                                    _multiplexer_data.id,
                                                                    _multiplexer_data.pin);
    }
}

////////////////////////////////////////////////////////////////////////////////
// DigitalSensorMapper
////////////////////////////////////////////////////////////////////////////////

DigitalSensorMapper::DigitalSensorMapper(const int index) :
    BaseSensorMapper(SensorType::DIGITAL_INPUT, index)
{}


CommandErrorCode DigitalSensorMapper::apply_command(const Command *cmd)
{
    // Handle digital-specific configurations
    CommandErrorCode status = CommandErrorCode::OK;
    switch(cmd->type())
    {
    case CommandType::SET_SENSOR_TYPE:
        {
            // This is set internally and not from the user, so just assert for bugs
            assert(static_cast<const SetSensorTypeCommand*>(cmd)->data() == SensorType::DIGITAL_INPUT);
        };
        break;

    default:
        status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
        break;

    }

    // If command was not handled, try to handle it in parent
    if (status == CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE)
    {
        return BaseSensorMapper::apply_command(cmd);
    }
    else
    {
        return status;
    }
}

void DigitalSensorMapper::put_config_commands_into(CommandIterator out_iterator)
{
    BaseSensorMapper::put_config_commands_into(out_iterator);
}

void DigitalSensorMapper::process(Value *value, output_backend::OutputBackend *backend)
{
    if (! _enabled)
    {
        return;
    }
    bool digital_val;
    if (value->type() == ValueType::DIGITAL)
    {
        digital_val = static_cast<DigitalValue*>(value)->value();
    }
    else if (value->type() == ValueType::ANALOG)
    {
        digital_val = static_cast<AnalogValue*>(value)->value() > 0;
    }
    else
    {
        return;
    }
    float out_val = digital_val? 1.0f : 0.0f;
    if (_invert_value)
    {
        out_val = 1.0f - out_val;
    }

    // Don't check for previous value changed on digital pins

    MessageFactory factory;
    // Use temporary variable here, since if the factory method is created inside the temporary rvalue expression
    // it gets optimized away by the compiler in release mode
    auto temp_msg = factory.make_output_value(_sensor_index,
                                              out_val,
                                              _send_timestamp? value->timestamp() : 0);
    auto transformed_value = static_cast<OutputValue*>(temp_msg.get());
    backend->send(transformed_value, value);
}

std::unique_ptr<Command> DigitalSensorMapper::process_set_value(Value*value)
{
    bool out_val;
    if (!_enabled)
    {
        return nullptr;
    }
    switch (value->type())
    {
        case ValueType::INT_SET:
            out_val = static_cast<IntegerSetValue*>(value)->value() > 0;
            break;

        case ValueType::FLOAT_SET:
            out_val = static_cast<FloatSetValue*>(value)->value() > 0.5f;
            break;

        default:
            return nullptr;
    }
    if (_invert_value)
    {
        out_val = !out_val;
    }
    return static_unique_ptr_cast<Command, BaseMessage>(_factory.make_set_digital_output_command(value->index(), out_val));
}

////////////////////////////////////////////////////////////////////////////////
// AnalogSensorMapper
////////////////////////////////////////////////////////////////////////////////

AnalogSensorMapper::AnalogSensorMapper(int index,
                                       float adc_sampling_rate) :
    BaseSensorMapper(SensorType::ANALOG_INPUT, index),
    _filter_time_constant(DEFAULT_FILTER_TIME_CONSTANT),
    _slider_threshold(0),
    _input_scale_range_low(0),
    _input_scale_range_high((1<<DEFAULT_ADC_BIT_RESOLUTION)-1),
    _adc_sampling_rate(adc_sampling_rate)
{
    _set_adc_bit_resolution(DEFAULT_ADC_BIT_RESOLUTION);
    _set_adc_filter_time_constant(DEFAULT_FILTER_TIME_CONSTANT);
}

CommandErrorCode AnalogSensorMapper::apply_command(const Command *cmd)
{

    // Handle analog-specific configurations, and fail if CommandType is not appropriate for this kind of sensor
    CommandErrorCode  status = CommandErrorCode::OK;
    switch(cmd->type())
    {
    // External
    case CommandType::SET_SENSOR_TYPE:
        {
            // This is set internally and not from the user, so just assert for bugs
            assert(static_cast<const SetSensorTypeCommand*>(cmd)->data() == SensorType::ANALOG_INPUT);
        };
        break;

    case CommandType::SET_SENSOR_HW_TYPE:
        {
            const auto typed_cmd = static_cast<const SetSensorHwTypeCommand*>(cmd);
            status = _set_sensor_hw_type(typed_cmd->data());
        };
        break;

    case CommandType::SET_ADC_BIT_RESOLUTION:
        {
            const auto typed_cmd = static_cast<const SetADCBitResolutionCommand*>(cmd);
            status = _set_adc_bit_resolution(typed_cmd->data());
        };
        break;

    case CommandType::SET_ADC_FILTER_TIME_CONSTANT:
        {
            const auto typed_cmd = static_cast<const SetADCFitlerTimeConstantCommand*>(cmd);
            status = _set_adc_filter_time_constant(typed_cmd->data());
        }
        break;

    case CommandType::SET_SLIDER_THRESHOLD:
        {
            const auto typed_cmd = static_cast<const SetSliderThresholdCommand*>(cmd);
            status = _set_slider_threshold(typed_cmd->data());
        };
        break;

    // Internal

    case CommandType::SET_INPUT_RANGE:
        {
            const auto typed_cmd = static_cast<const SetInputRangeCommand*>(cmd);
            auto range = typed_cmd->data();
            status = _set_input_scale_range(static_cast<int>(std::round(range.min)),
                                            static_cast<int>(std::round(range.max)));
        };
        break;

    default:
        status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
        break;

    }

    // If command was not handled, try to handle it in parent
    if (status == CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE)
    {
        return BaseSensorMapper::apply_command(cmd);
    }
    else
    {
        return status;
    }

}

void AnalogSensorMapper::put_config_commands_into(CommandIterator out_iterator)
{
    BaseSensorMapper::put_config_commands_into(out_iterator);

    MessageFactory factory;
    *out_iterator = factory.make_set_adc_bit_resolution_command(_sensor_index, _adc_bit_resolution);
    *out_iterator = factory.make_set_analog_time_constant_command(_sensor_index, _filter_time_constant);
    *out_iterator = factory.make_set_slider_threshold_command(_sensor_index, _slider_threshold);
    *out_iterator = factory.make_set_input_range_command(_sensor_index, _input_scale_range_low, _input_scale_range_high);
}

void AnalogSensorMapper::process(Value* value, output_backend::OutputBackend* backend)
{
    if (! _enabled)
    {
        return;
    }
    assert(value->type() == ValueType::ANALOG);

    auto analog_val = static_cast<AnalogValue*>(value);
    int clipped_val = clip<int>(analog_val->value(), _input_scale_range_low, _input_scale_range_high);
    float out_val =   static_cast<float>(clipped_val - _input_scale_range_low)
                    / static_cast<float>(_input_scale_range_high - _input_scale_range_low);
    if (_invert_value)
    {
        out_val = 1.0f - out_val;
    }
    if ((_sending_mode == SendingMode::ON_VALUE_CHANGED) && (fabsf(out_val - _previous_value) > PREVIOUS_VALUE_THRESHOLD))
    {
        MessageFactory factory;
        // Use temporary variable here, since if the factory method is created inside the temporary rvalue expression
        // it gets optimized away by the compiler in release mode
        auto temp_msg = factory.make_output_value(_sensor_index,
                                                  out_val,
                                                  _send_timestamp? value->timestamp() : 0);
        auto transformed_value = static_cast<OutputValue*>(temp_msg.get());
        backend->send(transformed_value, value);
        _previous_value = out_val;
    }
}

std::unique_ptr<Command> AnalogSensorMapper::process_set_value(Value*value)
{
    if (!_enabled)
    {
        return nullptr;
    }
    float out_val;
    switch (value->type())
    {
        case ValueType::FLOAT_SET:
            out_val = static_cast<FloatSetValue*>(value)->value();
            break;

        default:
            return nullptr;
    }
    out_val = clip<float>(out_val, 0.0f, 1.0f);
    if (_invert_value)
    {
        out_val = 1.0f - out_val;
    }
    out_val = out_val * (_input_scale_range_high - _input_scale_range_low) + _input_scale_range_low;
    return static_unique_ptr_cast<Command, BaseMessage>(_factory.make_set_range_output_command(value->index(), out_val));
}

CommandErrorCode AnalogSensorMapper::_set_sensor_hw_type(SensorHwType hw_type)
{
    _hw_type = hw_type;
    return CommandErrorCode::OK;
}

CommandErrorCode AnalogSensorMapper::_set_adc_bit_resolution(int resolution)
{
    if ((resolution < 1) || (resolution > MAX_ADC_BIT_RESOLUTION))
    {
        return CommandErrorCode::INVALID_VALUE;
    }

    _adc_bit_resolution = resolution;
    _max_allowed_input = (1 << _adc_bit_resolution) - 1;

    _input_scale_range_low = std::min(_input_scale_range_low, _max_allowed_input);
    _input_scale_range_high = std::min(_input_scale_range_high, _max_allowed_input);
    return CommandErrorCode::OK;
}


CommandErrorCode AnalogSensorMapper::_set_adc_filter_time_constant(float value)
{
    if (value <= 0.0f)
    {
        return CommandErrorCode::INVALID_VALUE;
    }

    _filter_time_constant = value;
    return CommandErrorCode::OK;
}

CommandErrorCode AnalogSensorMapper::_set_slider_threshold(int value)
{
    if ( (value < 0) || (value > (_max_allowed_input-1)) )
    {
        return CommandErrorCode::INVALID_VALUE;
    }

    _slider_threshold = value;
    return CommandErrorCode::OK;
}

CommandErrorCode AnalogSensorMapper::_set_input_scale_range(int low, int high)
{
    CommandErrorCode status = CommandErrorCode::OK;
    if ( (low < 0) || (high > (_max_allowed_input-1)))
    {
        return CommandErrorCode::INVALID_RANGE;
    }

    if (high <= low)
    {
        high = low + 1 ;
        status = CommandErrorCode::CLIP_WARNING;
    }

    _input_scale_range_high = high;
    _input_scale_range_low = low;
    return status;
}

////////////////////////////////////////////////////////////////////////////////
// RangeSensorMapper
////////////////////////////////////////////////////////////////////////////////
RangeSensorMapper::RangeSensorMapper(int index) : BaseSensorMapper(SensorType::ANALOG_INPUT, index),
                                                  _input_scale_range_low(0),
                                                  _input_scale_range_high(100)
{}

CommandErrorCode RangeSensorMapper::apply_command(const Command*cmd)
{
    CommandErrorCode  status = CommandErrorCode::OK;
    switch(cmd->type())
    {
        // External
        case CommandType::SET_SENSOR_TYPE:
        {
            // This is set internally and not from the user, so just assert for bugs
            assert(static_cast<const SetSensorTypeCommand*>(cmd)->data() == SensorType::RANGE_INPUT);
        };
        break;

        case CommandType::SET_SENSOR_HW_TYPE:
        {
            const auto typed_cmd = static_cast<const SetSensorHwTypeCommand*>(cmd);
            status = _set_sensor_hw_type(typed_cmd->data());
        };
        break;

        // Internal

        case CommandType::SET_INPUT_RANGE:
        {
            const auto typed_cmd = static_cast<const SetInputRangeCommand*>(cmd);
            auto range = typed_cmd->data();
            status = _set_input_scale_range(static_cast<int>(std::round(range.min)),
                                            static_cast<int>(std::round(range.max)));
        };
        break;

        break;

        default:
            status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
            break;

    }
    // If command was not handled, try to handle it in parent
    if (status == CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE)
    {
        return BaseSensorMapper::apply_command(cmd);
    }
    else
    {
        return status;
    }

}

void RangeSensorMapper::put_config_commands_into(CommandIterator out_iterator)
{
    BaseSensorMapper::put_config_commands_into(out_iterator);

    MessageFactory factory;
    *out_iterator = factory.make_set_input_range_command(_sensor_index, _input_scale_range_low, _input_scale_range_high);
}

void RangeSensorMapper::process(Value*value, output_backend::OutputBackend*backend)
{
    if (! _enabled)
    {
        return;
    }
    assert(value->type() == ValueType::ANALOG);

    auto range_value = static_cast<AnalogValue*>(value);
    auto out_val = clip<int>(range_value->value(), _input_scale_range_low, _input_scale_range_high);

    if (_invert_value)
    {
        out_val = _input_scale_range_high - out_val + _input_scale_range_low;
    }
    if (out_val != _previous_int_value)
    {
        MessageFactory factory;
        // Use temporary variable here, since if the factory method is created inside the temporary rvalue expression
        // it gets optimized away by the compiler in release mode
        auto temp_msg = factory.make_output_value(_sensor_index,
                                                  out_val,
                                                  _send_timestamp? value->timestamp() : 0);
        auto transformed_value = static_cast<OutputValue*>(temp_msg.get());
        backend->send(transformed_value, value);
        _previous_int_value = out_val;
    }
}

std::unique_ptr<Command> RangeSensorMapper::process_set_value(Value*value)
{
    if (!_enabled)
    {
        return nullptr;
    }
    int out_val;
    switch (value->type())
    {
        case ValueType::INT_SET:
            out_val = static_cast<IntegerSetValue*>(value)->value();
            break;

        case ValueType::FLOAT_SET:
            out_val = static_cast<int>(std::round(static_cast<FloatSetValue*>(value)->value()));
            break;

        default:
            return nullptr;
    }
    out_val = clip<int>(out_val, _input_scale_range_low, _input_scale_range_high);
    if (_invert_value)
    {
        out_val = _input_scale_range_high - out_val;
    }
    return static_unique_ptr_cast<Command, BaseMessage>(
            _factory.make_set_range_output_command(value->index(), out_val));
}

CommandErrorCode RangeSensorMapper::_set_sensor_hw_type(SensorHwType hw_type)
{
    _hw_type = hw_type;
    return CommandErrorCode::OK;
}

CommandErrorCode RangeSensorMapper::_set_input_scale_range(int low, int high)
{
    CommandErrorCode status = CommandErrorCode::OK;
    if (high <= low)
    {
        high = low + 1 ;
        status = CommandErrorCode::CLIP_WARNING;
    }

    _input_scale_range_low = low;
    _input_scale_range_high = high;
    return status;
}

////////////////////////////////////////////////////////////////////////////////
// ContinuousSensorMapper
////////////////////////////////////////////////////////////////////////////////

ContinuousSensorMapper::ContinuousSensorMapper(int index) :
        BaseSensorMapper(SensorType::CONTINUOUS_INPUT, index),
        _input_scale_range_low(-M_PI),
        _input_scale_range_high(M_PI)
{}

CommandErrorCode ContinuousSensorMapper::apply_command(const Command *cmd)
{
    // Handle digital-specific configurations
    CommandErrorCode status = CommandErrorCode::OK;
    SENSEI_LOG_INFO("ContinuousSensorMapper: Got a set sensor type command {}!", (int)cmd->type());
    switch(cmd->type())
    {
        case CommandType::SET_SENSOR_TYPE:
        {
            // This is set internally and not from the user, so just assert for bugs
            assert(static_cast<const SetSensorTypeCommand*>(cmd)->data() == SensorType::CONTINUOUS_INPUT);
            break;
        }
        case CommandType::SET_INPUT_RANGE:
        {
            const auto typed_cmd = static_cast<const SetInputRangeCommand*>(cmd);
            auto range = typed_cmd->data();
            status = _set_input_scale_range(range.min, range.max);
            break;
        }
        default:
            status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
            break;

    }

    // If command was not handled, try to handle it in parent
    if (status == CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE)
    {
        return BaseSensorMapper::apply_command(cmd);
    }
    else
    {
        return status;
    }

}

void ContinuousSensorMapper::put_config_commands_into(CommandIterator out_iterator)
{
    BaseSensorMapper::put_config_commands_into(out_iterator);

    MessageFactory factory;
    *out_iterator = factory.make_set_input_range_command(_sensor_index, _input_scale_range_low, _input_scale_range_high);
}

void ContinuousSensorMapper::process(Value *value, output_backend::OutputBackend *backend)
{
    if (! _enabled)
    {
        return;
    }
    assert(value->type() == ValueType::CONTINUOUS);

    auto continuous_val = static_cast<ContinuousValue*>(value);
    float clipped_val = clip<float>(continuous_val->value(), _input_scale_range_low, _input_scale_range_high);
    float out_val = (clipped_val - _input_scale_range_low) / (_input_scale_range_high - _input_scale_range_low);

    if (_invert_value)
    {
        out_val = 1.0f - out_val;
    }
    if (fabsf(out_val - _previous_value) > PREVIOUS_VALUE_THRESHOLD)
    {
        MessageFactory factory;
        // Use temporary variable here, since if the factory method is created inside the temporary rvalue expression
        // it gets optimized away by the compiler in release mode
        auto temp_msg = factory.make_output_value(_sensor_index,
                                                  out_val,
                                                  _send_timestamp? value->timestamp() : 0);
        auto transformed_value = static_cast<OutputValue*>(temp_msg.get());
        backend->send(transformed_value, value);
        _previous_value = out_val;
    }
}

std::unique_ptr<Command> ContinuousSensorMapper::process_set_value(Value*value)
{
    if (!_enabled)
    {
        return nullptr;
    }
    float out_val;
    switch (value->type())
    {
        case ValueType::FLOAT_SET:
            out_val = static_cast<FloatSetValue*>(value)->value();
            break;

        default:
            return nullptr;
    }
    out_val = clip<float>(out_val, 0.0f, 1.0f);
    if (_invert_value)
    {
        out_val = 1.0f - out_val;
    }
    out_val = out_val * (_input_scale_range_high - _input_scale_range_low) + _input_scale_range_low;
    return static_unique_ptr_cast<Command, BaseMessage>(_factory.make_set_continuous_output_command(value->index(), out_val));

}

CommandErrorCode ContinuousSensorMapper::_set_input_scale_range(float low, float high)
{
    CommandErrorCode status = CommandErrorCode::OK;
    if (high <= low)
    {
        low = high - 1;
        status = CommandErrorCode::CLIP_WARNING;
    }
    _input_scale_range_low = low;
    _input_scale_range_high = high;
    return status;
}