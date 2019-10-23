/*
 *  Copyright (c) 2019, The OpenThread Authors.
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

/**
 * @file
 *   This file implements a simple CLI for the CoAP service.
 */

#if OPENTHREAD_CONFIG_OTNS_ENABLE

#include "cli_udp.hpp"

#include <openthread/message.h>
#include <openthread/udp.h>

#include "cli/cli.hpp"
#include "cli/cli_otns.hpp"
#include "cli/cli_server.hpp"
#include "common/encoding.hpp"

#include <sys/time.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/otns.h>
#include <openthread/platform/radio.h>

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

const struct OTNS::Command OTNS::sCommands[] = {
    {"help", &OTNS::ProcessHelp},
    {"init", &OTNS::ProcessInit},
    {"ct", &OTNS::ProcessCollectionTree},
    {"send", &OTNS::ProcessSend},
};

OTNS::OTNS(Interpreter &aInterpreter)
    : mInterpreter(aInterpreter)
    , mCollectionTreeTimer(*mInterpreter.mInstance, HandleCollectionTreeTimer, this)
{
    memset(&mCTCoapResource, 0, sizeof(mCTCoapResource));
    memset(&mSendCoapResource, 0, sizeof(mSendCoapResource));
}

otError OTNS::ProcessHelp(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    for (unsigned int i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        mInterpreter.mServer->OutputFormat("%s\r\n", sCommands[i].mName);
    }

    return OT_ERROR_NONE;
}

otError OTNS::ProcessInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    otError error = OT_ERROR_NONE;

    error = otCoapStart(mInterpreter.mInstance, OT_DEFAULT_COAP_PORT);
    otLogInfoCli("OTNS: init: coap start: error=%d", error);

    mSendCoapResource.mUriPath = "ns/sd";
    mSendCoapResource.mContext = this;
    mSendCoapResource.mHandler = &OTNS::HandleCollectionTreeRequest;
    SuccessOrExit(error = otCoapAddResource(mInterpreter.mInstance, &mSendCoapResource));
exit:
    return error;
}

#define _MAX_SEND_DATA_SIZE (1024)

OT_TOOL_PACKED_BEGIN
struct send_data_t
{
    uint16_t data_size;
    int16_t  reply_size;
    uint8_t  data[_MAX_SEND_DATA_SIZE];
} OT_TOOL_PACKED_END;

otError OTNS::ProcessSend(int argc, char *argv[])
{
    send_data_t          send_data;
    Ip6::MessageInfo     sMessageInfo;
    const otMessageInfo *messageInfo = static_cast<const otMessageInfo *>(&sMessageInfo);
    otMessage *          message     = NULL;

    otError error = OT_ERROR_NONE;

    // Default parameters
    const otCoapType coapType = OT_COAP_TYPE_CONFIRMABLE;
    otCoapCode       coapCode = OT_COAP_CODE_POST;

    if (argc <= 1)
    {
        ExitNow(error = OT_ERROR_PARSE);
    }

    sMessageInfo = Ip6::MessageInfo();
    SuccessOrExit(error = sMessageInfo.GetPeerAddr().FromString(argv[0]));

    if (argc >= 3)
    {
        send_data.data_size = static_cast<uint16_t>(atoi(argv[2]));
        VerifyOrExit(send_data.data_size >= 0 && send_data.data_size <= _MAX_SEND_DATA_SIZE, error = OT_ERROR_PARSE);
    }

    send_data.reply_size = static_cast<int16_t>(send_data.data_size);

    if (argc >= 4)
    {
        send_data.reply_size = static_cast<int16_t>(atoi(argv[3]));
        VerifyOrExit(send_data.reply_size == -1 ||
                         (send_data.reply_size >= 0 && send_data.reply_size <= _MAX_SEND_DATA_SIZE),
                     error = OT_ERROR_PARSE);
    }

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, coapCode);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, "ns/sd"));

    SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));
    SuccessOrExit(
        error = otMessageAppend(message, &send_data,
                                sizeof(send_data.data_size) + sizeof(send_data.reply_size) + send_data.data_size));

    sMessageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    error = otCoapSendRequest(mInterpreter.mInstance, message, messageInfo, &OTNS::HandleCollectionTreeResponse, this);

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return OT_ERROR_NONE;
}

otError OTNS::ProcessCollectionTree(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;
    int     interval;
    int     sensor_id;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (strcmp(argv[0], "server") == 0)
    {
        mCTCoapResource.mUriPath = "ns/ct";
        mCTCoapResource.mContext = this;
        mCTCoapResource.mHandler = &OTNS::HandleCollectionTreeRequest;
        ExitNow(error = otCoapAddResource(mInterpreter.mInstance, &mCTCoapResource));
    }
    else if (strcmp(argv[0], "start") == 0)
    {
        if (argc < 3)
        {
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        sensor_id = atoi(argv[1]);
        VerifyOrExit(sensor_id > 0, error = OT_ERROR_INVALID_ARGS);

        interval = atoi(argv[2]);
        VerifyOrExit(interval > 0, error = OT_ERROR_INVALID_ARGS);

        mInterpreter.mServer->OutputFormat("starting, sensor_id: %d, interval: %d\r\n", sensor_id, interval);
        VerifyOrExit(!mCollectionTreeTimer.IsRunning(), error = OT_ERROR_ALREADY);

        mCollectionTreeSensorId   = static_cast<uint32_t>(sensor_id);
        mCollectionTreeIntervalMS = static_cast<uint32_t>(interval);
        mCollectionTreeTimer.Start(Random::NonCrypto::GetUint32InRange(0, mCollectionTreeIntervalMS));
        ExitNow(error = OT_ERROR_NONE);
    }
    else if (strcmp(argv[0], "stop") == 0)
    {
        mInterpreter.mServer->OutputFormat("stopping\r\n");
        mCollectionTreeTimer.Stop();
        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}

otError OTNS::Process(int argc, char *argv[])
{
    otError error = OT_ERROR_PARSE;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        error = OT_ERROR_INVALID_ARGS;
    }
    else
    {
        for (size_t i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
        {
            if (strcmp(argv[0], sCommands[i].mName) == 0)
            {
                error = (this->*sCommands[i].mCommand)(argc - 1, argv + 1);
                break;
            }
        }
    }
    return error;
}

OT_TOOL_PACKED_BEGIN
struct sense_data_t
{
    uint32_t sensor_id;
    uint32_t milli_time;
    uint32_t sense_val;
} OT_TOOL_PACKED_END;

void OTNS::HandleCollectionTreeTimer(Timer &aTimer)
{
    //    otLogCritCore("Sense & collect ...");
    TimerMilliContext &timer        = static_cast<TimerMilliContext &>(aTimer);
    OTNS &             otns         = *static_cast<OTNS *>(timer.GetContext());
    Interpreter &      mInterpreter = otns.mInterpreter;

    // reset the timer
    uint32_t interval = otns.mCollectionTreeIntervalMS;
    interval -= interval / 10;
    interval += Random::NonCrypto::GetUint32InRange(0, interval / 10 * 2);
    timer.Start(interval);

    // now, send the coap message

    otError       error   = OT_ERROR_NONE;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;

    // Default parameters
    const otCoapType coapType = OT_COAP_TYPE_CONFIRMABLE;
    otCoapCode       coapCode = OT_COAP_CODE_POST;
    otIp6Address     coapDestinationIp;

    // Destination IPv6 address
    //    const otMeshLocalPrefix *mlprefix = otThreadGetMeshLocalPrefix(mInterpreter.mInstance);
    SuccessOrExit(error = otThreadGetLeaderAloc(mInterpreter.mInstance, &coapDestinationIp));
    //    SuccessOrExit(error = otIp6AddressFromString("fd00:db8::ff:fe00:fc00", &coapDestinationIp));

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, coapType, coapCode);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, "ns/ct"));

    SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));

    sense_data_t sense_data;
    assert(otns.mCollectionTreeSensorId > 0);
    sense_data.sensor_id  = otns.mCollectionTreeSensorId;
    sense_data.milli_time = otPlatAlarmMilliGetNow();
    sense_data.sense_val  = 0xDDCCBBAA;

    SuccessOrExit(error = otMessageAppend(message, &sense_data, sizeof(sense_data_t)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = coapDestinationIp;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    error =
        otCoapSendRequest(mInterpreter.mInstance, message, &messageInfo, &OTNS::HandleCollectionTreeResponse, &otns);

exit:

    if ((error != OT_ERROR_NONE) && (message != NULL))
    {
        otMessageFree(message);
    }

    return;
}

void OTNS::HandleCollectionTreeRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<OTNS *>(aContext)->HandleCollectionTreeRequest(aMessage, aMessageInfo);
}

void OTNS::HandleCollectionTreeRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = NULL;
    otCoapCode responseCode    = OT_COAP_CODE_EMPTY;
    char       responseContent = '0';

    if (otThreadGetDeviceRole(mInterpreter.mInstance) != OT_DEVICE_ROLE_LEADER)
    {
        otLogWarnCoap("this is not the leader");
        return;
    }

    //    otLogCritCoap("HandleCollectionTreeRequest ...");
    assert(otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE);

    assert(otMessageGetOffset(aMessage) + sizeof(sense_data_t) == otMessageGetLength(aMessage));
    sense_data_t sense_data;
    VerifyOrExit(otMessageRead(aMessage, otMessageGetOffset(aMessage), &sense_data, sizeof(sense_data_t)) ==
                 sizeof(sense_data_t));

    //    otLogCritCoap("read senser %d data @%d = %x", sense_data.sensor_id, sense_data.milli_time,
    //                  HostSwap32(sense_data.sense_val));

    OTNS_STATUS_PUSH("ctss=%x:%x:%x", sense_data.sensor_id, sense_data.milli_time, sense_data.sense_val);

    responseCode = OT_COAP_CODE_VALID;

    responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, responseCode);
    SuccessOrExit(error = otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage),
                                                otCoapMessageGetTokenLength(aMessage)));

    if (otCoapMessageGetCode(aMessage) == OT_COAP_CODE_GET)
    {
        SuccessOrExit(error = otCoapMessageSetPayloadMarker(responseMessage));
        SuccessOrExit(error = otMessageAppend(responseMessage, &responseContent, sizeof(responseContent)));
    }

    SuccessOrExit(error = otCoapSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE)
    {
        if (responseMessage != NULL)
        {
            otMessageFree(responseMessage);
        }
        otLogCritCoap("coap send response error: %d %s", error, otThreadErrorToString(error));
    }
    else if (responseCode >= OT_COAP_CODE_RESPONSE_MIN)
    {
        //        mInterpreter.mServer->OutputFormat("coap response sent\r\n");
    }
}

void OTNS::HandleCollectionTreeResponse(void *               aContext,
                                        otMessage *          aMessage,
                                        const otMessageInfo *aMessageInfo,
                                        otError              aError)
{
    static_cast<OTNS *>(aContext)->HandleCollectionTreeResponse(aMessage, aMessageInfo, aError);
}

void OTNS::HandleCollectionTreeResponse(otMessage *aMessage, const otMessageInfo *aMessageInfo, otError aError)
{
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aMessageInfo);

    if (aError != OT_ERROR_NONE)
    {
        otLogWarnCoap("coap receive response error %d: %s\r\n", aError, otThreadErrorToString(aError));
    }
    else
    {
    }
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_OTNS_ENABLE
