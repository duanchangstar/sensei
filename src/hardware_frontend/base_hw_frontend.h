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
 * @brief Base interface for SENSEI to perform the logic of the GPIO PROTOCOL
 *        master.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
#ifndef SENSEI_BASE_HW_FRONTEND_H
#define SENSEI_BASE_HW_FRONTEND_H

#include <memory>

#include "synchronized_queue.h"
#include "message/base_message.h"
#include "message/base_command.h"

namespace sensei {
namespace hw_frontend {

/**
 * @brief Base class for frontends connecting to HW
 */
class BaseHwFrontend
{
public:
    /**
     * @brief Class constructor
     * @param [in] in_queue Output queue where incoming messages go
     * @param [in] out_queue Queue for messages to be sent to HW
    */
    BaseHwFrontend(SynchronizedQueue<std::unique_ptr<Command>>*in_queue,
                   SynchronizedQueue<std::unique_ptr<BaseMessage>>*out_queue)
    {
        _in_queue = in_queue;
        _out_queue = out_queue;
    }

    virtual ~BaseHwFrontend() = default;

    /**
     * @brief Spawn new threads for reading continuously from the port and in_queue
     */
    virtual void run() = 0;

    /**
     * @brief Stops the read and write threads if they are running
     */
    virtual void stop() = 0;

    /**
     * @brief Stops the flow of messages. If set to true, incoming packets
     * are silently dropped
     * @param [in] enabled Sets mute enabled/disabled
     */
    virtual void mute(bool enabled) = 0;

    /**
     * @brief Enables tracking and verification of packets sent
     * @param [in] enabled Sets ack verification enabled/disabled
     */
    virtual void verify_acks(bool enabled) = 0;

protected:
    SynchronizedQueue<std::unique_ptr<Command>>*_in_queue;
    SynchronizedQueue<std::unique_ptr<BaseMessage>>*_out_queue;
};


class NoOpFrontend : public BaseHwFrontend
{
public:
    NoOpFrontend(SynchronizedQueue<std::unique_ptr<Command>>*in_queue,
                 SynchronizedQueue<std::unique_ptr<BaseMessage>>*out_queue) : BaseHwFrontend(in_queue, out_queue)
    {}
    void run() {}
    void stop() {}
    void mute(bool /*enabled*/) {}
    void verify_acks(bool /*enabled*/) {}
};

}; // namespace hw_frontend
}; // namespace sensei

#endif //SENSEI_BASE_HW_FRONTEND_H