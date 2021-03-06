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
 * @brief OSC runtime user frontend
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#include "osc_user_frontend.h"
#include "logging.h"

#include <sstream>

using namespace sensei;
using namespace sensei::user_frontend;

SENSEI_GET_LOGGER_WITH_MODULE_NAME("osc_frontend");

namespace
{
static const int DEFAULT_SERVER_PORT = 23024;

static void osc_error(int num, const char *msg, const char *path)
{
    SENSEI_LOG_ERROR("liblo server error {} in path {}: {}", num, path, msg);
}

static int osc_set_sensor_enabled(const char* /*path*/, const char* /*types*/, lo_arg**argv, int /*argc*/,
                                  void* /*data*/, void*user_data)
{
    OSCUserFrontend *self = static_cast<OSCUserFrontend*>(user_data);
    int pin_idx = argv[0]->i;
    bool enabled = static_cast<bool>(argv[1]->i);
    self->set_enabled(pin_idx, enabled);
    SENSEI_LOG_DEBUG("Setting pin {} to enabled status {}", pin_idx, enabled);

    return 0;
}

static int osc_set_digital_output(const char* /*path*/, const char* /*types*/, lo_arg ** argv, int /*argc*/, void* /*data*/, void *user_data)
{
    OSCUserFrontend *self = static_cast<OSCUserFrontend*>(user_data);
    int id = argv[0]->i;
    bool value = static_cast<bool>(argv[1]->i);
    self->set_digital_output(id, value);
    SENSEI_LOG_DEBUG("Sending value {} to digital output {}", value, id);

    return 0;
}

static int osc_set_continuous_output(const char* /*path*/, const char* /*types*/, lo_arg ** argv, int /*argc*/, void* /*data*/, void *user_data)
{
    OSCUserFrontend *self = static_cast<OSCUserFrontend*>(user_data);
    int id = argv[0]->i;
    float value = argv[1]->f;
    self->set_continuous_output(id, value);
    SENSEI_LOG_DEBUG("Sending value {} to output {}", value, id);

    return 0;
}

static int osc_set_range_output(const char* /*path*/, const char* /*types*/, lo_arg ** argv, int /*argc*/, void* /*data*/, void *user_data)
{
    OSCUserFrontend *self = static_cast<OSCUserFrontend*>(user_data);
    int id = argv[0]->i;
    int value = argv[1]->i;
    self->set_range_output(id, value);
    SENSEI_LOG_DEBUG("Sending value {} to range output {}", value, id);

    return 0;
}

}; // anonymous namespace

OSCUserFrontend::OSCUserFrontend(SynchronizedQueue<std::unique_ptr<BaseMessage>> *queue,
                                 const int max_n_input_pins,
                                 const int max_n_digital_out_pins) :
        UserFrontend(queue, max_n_input_pins, max_n_digital_out_pins),
            _osc_server(nullptr),
            _server_port(DEFAULT_SERVER_PORT)
{
    _start_server();
}

CommandErrorCode OSCUserFrontend::apply_command(const Command* cmd)
{
    CommandErrorCode status = CommandErrorCode::OK;

    switch(cmd->type())
    {

    case CommandType::SET_OSC_INPUT_PORT:
        {
            const auto typed_cmd = static_cast<const SetOSCInputPortCommand*>(cmd);
            auto port = typed_cmd->data();
            if ((port < 1000) || (port > 65535))
            {
                status = CommandErrorCode::INVALID_PORT_NUMBER;
            }
            else
            {
                _server_port = port;
                _stop_server();
                _start_server();
            }
        };
        break;

    default:
        status = CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE;
        break;
    }

    // If command was not handled, try in the parent class
    if (status == CommandErrorCode::UNHANDLED_COMMAND_FOR_SENSOR_TYPE)
    {
        return UserFrontend::apply_command(cmd);
    }
    else
    {
        return status;
    }
}

void OSCUserFrontend::_start_server()
{
    std::stringstream port_stream;
    port_stream << _server_port;

    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
    lo_server_thread_add_method(_osc_server, "/set_enabled", "ii", osc_set_sensor_enabled, this);
    lo_server_thread_add_method(_osc_server, "/set_output", "if", osc_set_continuous_output, this);
    int ret = lo_server_thread_start(_osc_server);
    if (ret < 0)
    {
        SENSEI_LOG_ERROR("Error {} while starting OSC server thread", ret);
    }
}

void OSCUserFrontend::_stop_server()
{
    int ret = lo_server_thread_stop(_osc_server);
    if (ret < 0)
    {
        SENSEI_LOG_ERROR("Error {} while stopping OSC server thread", ret);
    }
    lo_server_thread_free(_osc_server);
}