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

#ifndef HDLC_HPP_
#define HDLC_HPP_

#include <common/message.hpp>
#include <common/thread_error.hpp>

namespace Thread {

class Hdlc
{
public:
    typedef void (*ReceiveHandler)(void *context, uint8_t protocol, uint8_t *frame, uint16_t frameLength);
    typedef void (*SendDoneHandler)(void *context);
    typedef void (*SendMessageDoneHandler)(void *context);

    static ThreadError Init(void *context, ReceiveHandler receiveHandler, SendDoneHandler sendDoneHandler,
                            SendMessageDoneHandler sendMessageDoneHandler);
    static ThreadError Start();
    static ThreadError Stop();

    static ThreadError Send(uint8_t protocol, uint8_t *frame, uint16_t frameLength);
    static ThreadError SendMessage(uint8_t protocol, Message &message);
};

}  // namespace Thread

#endif  // HDLC_HPP_
