/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 * @file
 *   This file contains definitions for a CLI server on a UDP socket.
 */

#ifndef CLI_UDP_HPP_
#define CLI_UDP_HPP_

#include <openthread.h>
#include <cli/cli_server.hpp>
#include <common/thread_error.hpp>

namespace Thread {
namespace Cli {

class Command;

class Udp: public Server
{
public:
    ThreadError Start() final;
    ThreadError Output(const char *buf, uint16_t bufLength) final;

private:
    static void HandleUdpReceive(void *context, otMessage message, const otMessageInfo *messageInfo);
    void HandleUdpReceive(otMessage message, const otMessageInfo *messageInfo);

    otUdpSocket mSocket;
    otMessageInfo mPeer;
};

}  // namespace Cli
}  // namespace Thread

#endif  // CLI_UDP_HPP_
