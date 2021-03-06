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
 * @brief Output backend using standard output/error streams
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#include <cstdio>
#include <cassert>

#include "std_stream_backend.h"

using namespace sensei;
using namespace sensei::output_backend;

StandardStreamBackend::StandardStreamBackend(const int max_n_input_pins) :
        OutputBackend(max_n_input_pins)
{
}

void StandardStreamBackend::send(const OutputValue* transformed_value, const Value* raw_input_value)
{
    int sensor_index = transformed_value->index();

    if (_send_output_active)
    {
        printf("Pin: %d, name: %s, value: %f\n", sensor_index,
                                                 _sensor_names[sensor_index].c_str(),
                                                 transformed_value->value());
    }

    if (_send_raw_input_active)
    {
        assert(raw_input_value != nullptr);
        switch (raw_input_value->type())
        {
        case ValueType::ANALOG:
            {
                auto typed_val = static_cast<const AnalogValue *>(raw_input_value);
                fprintf(stderr, "--RAW INPUT-- Pin: %d, name: %s, value: %d\n", sensor_index,
                                                                                _sensor_names[sensor_index].c_str(),
                                                                                typed_val->value());
            }
            break;

        case ValueType::DIGITAL:
            {
                auto typed_val = static_cast<const DigitalValue *>(raw_input_value);
                fprintf(stderr, "--RAW INPUT-- Pin: %d, name: %s, value: %d\n", sensor_index,
                                                                                _sensor_names[sensor_index].c_str(),
                                                                                typed_val->value());
            }
            break;

        case ValueType::CONTINUOUS:
            {
                auto  typed_val = static_cast<const ContinuousValue*>(raw_input_value);
                fprintf(stderr, "--RAW INPUT-- Pin: %d, name: %s, value: %f\n", sensor_index,
                                                                                _sensor_names[sensor_index].c_str(),
                                                                                typed_val->value());
            }
            break;

        default:
            break;
        }
    }

}

CommandErrorCode StandardStreamBackend::apply_command(const Command *cmd)
{
    // no specific commands for this subclass

    return OutputBackend::apply_command(cmd);

}