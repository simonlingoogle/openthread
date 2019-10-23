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
 *   This file implements CLI commands used by OTNS.
 */

#include "cli_otns.hpp"

#if OPENTHREAD_CONFIG_OTNS_ENABLE

#include <sys/time.h>
#include <openthread/message.h>

#include "cli/cli_server.hpp"
#include "common/logging.hpp"
#include "utils/otns.hpp"

using ot::Encoding::BigEndian::HostSwap16;

namespace ot {
namespace Cli {

const struct OTNS::Command OTNS::sCommands[] = {
    {"help", &OTNS::ProcessHelp},
    {"init", &OTNS::ProcessInit},
    {"ct", &OTNS::ProcessCollectionTree},
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

    return error;
}

otError OTNS::ProcessCollectionTree(int argc, char *argv[])
{
    otError  error = OT_ERROR_PARSE;
    uint32_t interval;
    uint32_t sensorId;

    if (argc < 1)
    {
        ProcessHelp(0, NULL);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (strcmp(argv[0], "server") == 0)
    {
        mCTCoapResource.mUriPath = OTNS_URI_PATH_COLLECTION_TREE;
        mCTCoapResource.mContext = this;
        mCTCoapResource.mHandler = &OTNS::HandleCollectionTreeRequest;
        ExitNow(error = otCoapAddResource(mInterpreter.mInstance, &mCTCoapResource));
    }
    else if (strcmp(argv[0], "start") == 0)
    {
        unsigned long value;

        VerifyOrExit(argc >= 3, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = Interpreter::ParseUnsignedLong(argv[1], value));
        sensorId = static_cast<uint32_t>(value);
        VerifyOrExit(sensorId > 0, error = OT_ERROR_INVALID_ARGS);

        SuccessOrExit(error = Interpreter::ParseUnsignedLong(argv[2], value));
        interval = static_cast<uint32_t>(value);
        VerifyOrExit(interval > 0, error = OT_ERROR_INVALID_ARGS);

        mInterpreter.mServer->OutputFormat("starting, sensorId: %d, interval: %d\r\n", sensorId, interval);
        VerifyOrExit(!mCollectionTreeTimer.IsRunning(), error = OT_ERROR_ALREADY);

        mCollectionTreeSensorId   = static_cast<uint32_t>(sensorId);
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

void OTNS::HandleCollectionTreeTimer()
{
    otError       error;
    otMessage *   message = NULL;
    otMessageInfo messageInfo;
    SenseData     senseData;

    // Default parameters
    otIp6Address coapDestinationIp;

    // reset the timer with random interval in [0.9 * mCollectionTreeIntervalMS, 1.1 * mCollectionTreeIntervalMS]
    uint32_t interval = mCollectionTreeIntervalMS;

    interval += Random::NonCrypto::GetUint32InRange(0, interval / 10 * 2) - interval / 10;
    mCollectionTreeTimer.Start(interval);

    // now, send the coap message
    SuccessOrExit(error = GetBorderRouterAddress(coapDestinationIp));
    //    SuccessOrExit(error = otThreadGetLeaderRloc(mInterpreter.mInstance, &coapDestinationIp));

    message = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(message != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInit(message, OT_COAP_TYPE_CONFIRMABLE, OT_COAP_CODE_POST);
    otCoapMessageGenerateToken(message, ot::Coap::Message::kDefaultTokenLength);
    SuccessOrExit(error = otCoapMessageAppendUriPathOptions(message, OTNS_URI_PATH_COLLECTION_TREE));

    SuccessOrExit(error = otCoapMessageSetPayloadMarker(message));

    assert(mCollectionTreeSensorId > 0);
    senseData.sensorId  = HostSwap32(mCollectionTreeSensorId);
    senseData.milliTime = HostSwap32(otPlatAlarmMilliGetNow());
    senseData.senseVal  = HostSwap32(0xAABBCCDD);

    SuccessOrExit(error = otMessageAppend(message, &senseData, sizeof(senseData)));

    memset(&messageInfo, 0, sizeof(messageInfo));
    messageInfo.mPeerAddr = coapDestinationIp;
    messageInfo.mPeerPort = OT_DEFAULT_COAP_PORT;

    error = otCoapSendRequest(mInterpreter.mInstance, message, &messageInfo, NULL, this);

exit:

    if (error != OT_ERROR_NONE)
    {
        if ((message != NULL))
        {
            otMessageFree(message);
        }
        otLogWarnCli("OTNS handle collection timer failed: error=%d", error);
    }

    return;
}

void OTNS::HandleCollectionTreeTimer(Timer &aTimer)
{
    TimerMilliContext &timer = static_cast<TimerMilliContext &>(aTimer);
    OTNS &             otns  = *static_cast<OTNS *>(timer.GetContext());
    otns.HandleCollectionTreeTimer();
}

void OTNS::HandleCollectionTreeRequest(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    static_cast<OTNS *>(aContext)->HandleCollectionTreeRequest(aMessage, aMessageInfo);
}

void OTNS::HandleCollectionTreeRequest(otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    otError    error           = OT_ERROR_NONE;
    otMessage *responseMessage = NULL;
    SenseData  senseData;

    VerifyOrExit(otCoapMessageGetType(aMessage) == OT_COAP_TYPE_CONFIRMABLE, error = OT_ERROR_PARSE);
    VerifyOrExit(otCoapMessageGetCode(aMessage) == OT_COAP_CODE_POST, error = OT_ERROR_PARSE);

    VerifyOrExit(otMessageRead(aMessage, otMessageGetOffset(aMessage), &senseData, sizeof(SenseData)) ==
                     sizeof(SenseData),
                 error = OT_ERROR_PARSE);

    OtnsStatusPush("ctss=%lx,%lx,%lx", HostSwap32(senseData.sensorId), HostSwap32(senseData.milliTime),
                   HostSwap32(senseData.senseVal));

    responseMessage = otCoapNewMessage(mInterpreter.mInstance, NULL);
    VerifyOrExit(responseMessage != NULL, error = OT_ERROR_NO_BUFS);

    otCoapMessageInitResponse(responseMessage, aMessage, OT_COAP_TYPE_ACKNOWLEDGMENT, OT_COAP_CODE_CHANGED);
    SuccessOrExit(error = otCoapMessageSetToken(responseMessage, otCoapMessageGetToken(aMessage),
                                                otCoapMessageGetTokenLength(aMessage)));

    SuccessOrExit(error = otCoapSendResponse(mInterpreter.mInstance, responseMessage, aMessageInfo));

exit:

    if (error != OT_ERROR_NONE)
    {
        if (responseMessage != NULL)
        {
            otMessageFree(responseMessage);
        }
        otLogWarnCli("OTNS handle %s failed: %d %s", OTNS_URI_PATH_COLLECTION_TREE, error,
                     otThreadErrorToString(error));
    }
}

otError OTNS::GetBorderRouterAddress(otIp6Address &aAddr)
{
    otError               error    = OT_ERROR_NOT_FOUND;
    otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
    otBorderRouterConfig  config;

    while (otNetDataGetNextOnMeshPrefix(mInterpreter.mInstance, &iterator, &config) == OT_ERROR_NONE)
    {
        error = otThreadGetRlocFromRloc16(mInterpreter.mInstance, config.mRloc16, &aAddr);
        ExitNow();
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnCli("Get border router address failed: %s", otThreadErrorToString(error));
    }

    return error;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_OTNS_ENABLE
