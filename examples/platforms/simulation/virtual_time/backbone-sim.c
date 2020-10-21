/*
 *  Copyright (c) 2020, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform-simulation.h"

#if OPENTHREAD_SIMULATION_VIRTUAL_TIME

#include <openthread/icmp6.h>
#include <openthread/ip6.h>
#include <openthread/udp.h>

void handleIp6DatagramFromThread(otMessage *aMessage, void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    struct Event event;
    uint16_t     length = otMessageGetLength(aMessage);
    uint16_t     readLength;

    assert(length <= OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH);

    readLength = otMessageRead(aMessage, 0, &event.mData, OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH);
    assert(readLength == length);

    otMessageFree(aMessage);

    event.mDataLength = length;
    event.mDelay      = 1;
    event.mEvent      = OT_SIM_EVENT_IP6_DATAGRAM;

    otSimSendEvent(&event);
}

void forwardUdp(otMessage *aMessage, uint16_t aPeerPort, otIp6Address *aPeerAddr, uint16_t aSockPort, void *aContext)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aPeerPort);
    OT_UNUSED_VARIABLE(aPeerAddr);
    OT_UNUSED_VARIABLE(aSockPort);
    OT_UNUSED_VARIABLE(aContext);

    struct Event event;
    uint16_t     length = otMessageGetLength(aMessage);
    uint16_t     offset = 0;
    uint16_t     readLength;

    memcpy(&event.mData[offset], aPeerAddr, sizeof(*aPeerAddr));
    offset += sizeof(*aPeerAddr);

    *((uint16_t *)(&event.mData[offset])) = htons(aPeerPort);
    offset += sizeof(uint16_t);

    *((uint16_t *)(&event.mData[offset])) = htons(aSockPort);
    offset += sizeof(uint16_t);

    readLength = otMessageRead(aMessage, 0, &event.mData[offset], length);
    assert(readLength == length);
    offset += length;

    otMessageFree(aMessage);

    event.mDataLength = offset;
    event.mDelay      = 1;
    event.mEvent      = OT_SIM_EVENT_UDP_FORWARD;

    otSimSendEvent(&event);
}

void platformBackboneInit(otInstance *aInstance)
{
    otIp6SetReceiveCallback(aInstance, handleIp6DatagramFromThread, NULL);
    otUdpForwardSetForwarder(aInstance, forwardUdp, NULL);
    otIp6SetReceiveFilterEnabled(aInstance, true);
    otIcmp6SetEchoMode(aInstance, OT_ICMP6_ECHO_HANDLER_DISABLED);
}

void platformUdpForward(const otIp6Address *aDstAddr,
                        uint16_t            aDstPort,
                        uint16_t            aSrcPort,
                        const uint8_t *     aData,
                        uint16_t            aLength)
{
    OT_UNUSED_VARIABLE(aDstAddr);
    OT_UNUSED_VARIABLE(aDstPort);
    OT_UNUSED_VARIABLE(aSrcPort);
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aLength);
}

void platformIp6DatagramReceive(uint8_t *aData, uint16_t aLength)
{
    OT_UNUSED_VARIABLE(aData);
    OT_UNUSED_VARIABLE(aLength);
}

void otSysSetupInstance(otInstance *aInstance)
{
    platformBackboneInit(aInstance);
}

#endif // OPENTHREAD_SIMULATION_VIRTUAL_TIME
