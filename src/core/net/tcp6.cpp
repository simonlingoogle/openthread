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

/**
 * @file
 *   This file implements TCP/IPv6 sockets.
 */

#include "tcp6.hpp"

#if (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_TCP_ENABLE

#include <new>

#include <common/logging.hpp>

#include "common/code_utils.hpp"
#include "common/encoding.hpp"
#include "common/instance.hpp"
#include "common/locator-getters.hpp"
#include "net/checksum.hpp"
#include "net/ip6.hpp"

namespace ot {
namespace Ip6 {

using Socket   = Tcp::Socket;
using Sequence = Tcp::Sequence;

#if OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
#define CHECK_TCP_INVARIANT(cond) OT_ASSERT(cond)
#else
#define CHECK_TCP_INVARIANT(cond)
#endif

otError Tcp::Open(Socket &aSocket, otTcpEventHandler aEventHandler, void *aContext)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(!mSockets.Contains(aSocket), error = OT_ERROR_ALREADY);

    new (&aSocket) Socket(GetInstance(), aEventHandler, aContext);

exit:
    return error;
}

void Tcp::RemoveSocket(Socket &aSocket)
{
    Socket *prev;
    otError error;

    error = mSockets.Find(aSocket, prev);
    OT_ASSERT(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(error);

    mSockets.PopAfter(prev);
    aSocket.SetNext(nullptr);
}

uint16_t Tcp::GetEphemeralPort(void)
{
    uint16_t rval = mEphemeralPort;

    if (mEphemeralPort < kDynamicPortMax)
    {
        mEphemeralPort++;
    }
    else
    {
        mEphemeralPort = kDynamicPortMin;
    }

    return rval;
}

Message *Tcp::NewMessage(const Message::Settings &aSettings)
{
    Message *message = Get<Ip6>().NewMessage(0, aSettings);
    otError  error;

    VerifyOrExit(message != nullptr);

    message->SetIsManagedByTcp(true);
    error = message->SetLength(sizeof(Header));
    OT_ASSERT(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(error);

    message->SetOffset(sizeof(Header));

exit:
    return message;
}

void Tcp::AddSocket(Socket &aSocket)
{
    OT_ASSERT(!mSockets.Contains(aSocket));

    IgnoreError(mSockets.Add(aSocket));
}

Tcp::Tcp(Instance &aInstance)
    : InstanceLocator(aInstance)
    , mSoleTimer(aInstance, Tcp::HandleSoleTimer)
    , mEphemeralPort(kDynamicPortMin)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mSegmentRandomDropProb(0)
#endif
{
}

void Tcp::StopTimer(Tcp::TcpTimer &aTimer)
{
    if (mTimerList.Remove(aTimer) == OT_ERROR_NONE)
    {
        ResetSoleTimer();
    }
}

void Tcp::StartTimer(Tcp::TcpTimer &aTimer, TimeMilli aFireTime)
{
    TcpTimer *prev = nullptr;

    StopTimer(aTimer);
    aTimer.SetFireTime(aFireTime);

    for (TcpTimer *cur = mTimerList.GetHead(); cur; prev = cur, cur = cur->GetNext())
    {
        if (aTimer.GetFireTime() < cur->GetFireTime())
        {
            break;
        }
    }

    if (prev == nullptr)
    {
        mTimerList.Push(aTimer);
    }
    else
    {
        mTimerList.PushAfter(aTimer, *prev);
    }

    ResetSoleTimer();
}

void Tcp::HandleSoleTimer(Timer &aTimer)
{
    aTimer.GetInstance().Get<Tcp>().HandleSoleTimer();
}

void Tcp::HandleSoleTimer(void)
{
    TimeMilli now = TimerMilli::GetNow();

    while (mTimerList.GetHead() != nullptr)
    {
        VerifyOrExit(mTimerList.GetHead()->GetFireTime() <= now);

        {
            TcpTimer *timer = mTimerList.Pop();
            timer->GetSocket().HandleTimer();
        }
    }
exit:
    return;
}

void Tcp::ResetSoleTimer(void)
{
    if (mTimerList.IsEmpty())
    {
        mSoleTimer.Stop();
    }
    else
    {
        mSoleTimer.FireAt(mTimerList.GetHead()->GetFireTime());
    }
}

void Socket::SetState(otTcpState aState)
{
    bool       isDisconnect = false;
    otTcpState prevState    = mState;

    OT_ASSERT(mState != aState);

    otLogDebgTcp("TCP %s,%d<-%s,%d state changed: %s -> %s", GetSockName().GetAddress().ToString().AsCString(),
                 GetSockName().mPort, GetPeerName().GetAddress().ToString().AsCString(), GetPeerName().mPort,
                 StateToString(mState), StateToString(aState));

    mState = aState;

    if (mState == OT_TCP_STATE_TIME_WAIT)
    {
        mTimeWaitStartTime = TimerMilli::GetNow();
    }

    CHECK_TCP_INVARIANT(
        !(mState == OT_TCP_STATE_FIN_WAIT_2 || mState == OT_TCP_STATE_TIME_WAIT || mState == OT_TCP_STATE_CLOSED) ||
        GetSendQueue().IsEmpty());
    CHECK_TCP_INVARIANT(!(mState == OT_TCP_STATE_CLOSING || mState == OT_TCP_STATE_TIME_WAIT ||
                          mState == OT_TCP_STATE_CLOSE_WAIT || mState == OT_TCP_STATE_LAST_ACK ||
                          mState == OT_TCP_STATE_CLOSED) ||
                        GetRecvQueue().IsProcessEmpty());

    isDisconnect =
        (mState == OT_TCP_STATE_TIME_WAIT || (mState == OT_TCP_STATE_CLOSED && prevState != OT_TCP_STATE_TIME_WAIT));

    if (mState == OT_TCP_STATE_ESTABLISHED)
    {
        TriggerEvent(OT_TCP_SOCKET_CONNECTED);
    }

    if (isDisconnect)
    {
        TriggerEvent(OT_TCP_SOCKET_DISCONNECTED);
    }

    ResetTimer();

    if (mState == OT_TCP_STATE_CLOSED)
    {
        GetRecvQueue().Flush();
        TriggerEvent(OT_TCP_SOCKET_CLOSED);
        this->~Socket();
    }
}

bool Socket::IsAckAcceptable(Sequence aAckNumber)
{
    bool acceptable = false;

    // Never accept ACK in CLOSED or LISTEN states
    VerifyOrExit(mState != OT_TCP_STATE_CLOSED && mState != OT_TCP_STATE_LISTEN);
    // Verify SND.UNA < SEG.ACK =< SND.NXT
    acceptable = GetSndUna() <= aAckNumber && aAckNumber <= GetSndNxt();

exit:
    return acceptable;
}

bool Socket::IsSeqAcceptable(const Header &aHeader, uint16_t aSegmentLength, bool &aIsDuplicate)
{
    // There are four cases for the acceptability test for an incoming
    // segment:
    //
    // Segment Receive  Test
    // Length  Window
    // ------- -------  -------------------------------------------
    //
    //    0       0     SEG.SEQ = RCV.NXT
    //
    //    0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
    //
    //   >0       0     not acceptable
    //
    //   >0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
    //               or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND

    Sequence rcvNxt    = GetRcvNxt();
    Sequence rcvWinEnd = rcvNxt + GetReceiveWindowSize();
    Sequence startSeq  = aHeader.GetSequenceNumber();
    Sequence stopSeq   = startSeq + aSegmentLength;
    bool     acceptable;

    if (rcvWinEnd == rcvNxt)
    {
        // Receive Window is 0
        acceptable = aSegmentLength == 0 && startSeq == rcvNxt;
    }
    else if (aSegmentLength == 0)
    {
        // Receive Window > 0, Segment Length == 0
        acceptable = rcvNxt <= startSeq && startSeq < rcvWinEnd;
    }
    else
    {
        // Receive Window > 0, Segment Length > 0
        acceptable = ((rcvNxt <= startSeq && startSeq < rcvWinEnd) || (rcvNxt < stopSeq && stopSeq <= rcvWinEnd));
    }

    aIsDuplicate = startSeq < rcvNxt && stopSeq <= rcvNxt;

    return acceptable;
}

uint16_t Socket::GetReceiveWindowSize(void)
{
    uint16_t window;
    uint16_t bufLimit;

    VerifyOrExit(Get<MessagePool>().GetFreeBufferCount() > kMinFreeMessageBufferThreshold, window = 0);

    window = GetRecvQueue().GetReceiveWindowSize();

    bufLimit = kMaxSegmentSizeNoFrag * (Get<MessagePool>().GetFreeBufferCount() - kMinFreeMessageBufferThreshold);
    window   = OT_MIN(window, bufLimit);

exit:
    return window;
}

void Socket::ProcessAck(const Header &aHeader)
{
    uint32_t rtt;
    Sequence ack               = aHeader.GetAcknowledgmentNumber();
    uint16_t oldSendWindowSize = GetSendQueue().GetSendWindowSize();
    bool     notifyDataSent    = false;

    CHECK_TCP_INVARIANT(IsAckAcceptable(ack));

    // If SND.UNA < SEG.ACK =< SND.NXT, the send window should be updated.
    GetSendQueue().UpdateSendWindow(aHeader);
    notifyDataSent = GetSendQueue().GetSendWindowSize() > oldSendWindowSize;

    if (GetSendQueue().ReclaimAcked(ack, rtt) > 0)
    {
        ProcessFINAcked();
        UpdateRTT(rtt);
        notifyDataSent = true;
    }

    if (notifyDataSent)
    {
        NotifyDataSent();
    }
}

uint32_t Socket::GetRTT(void) const
{
    uint32_t rtt = mSmoothedRTT * kRTTBetaNumerator / kRTTBetaDenominator;
    rtt          = OT_MIN(rtt, static_cast<uint32_t>(mMaxRoundTripTime));
    rtt          = OT_MAX(rtt, static_cast<uint32_t>(mMinRoundTripTime));
    otLogDebgTcp("GetRTT %u => %u", mSmoothedRTT, rtt);
    return rtt;
}

void Socket::UpdateRTT(uint32_t aRTT)
{
    mSmoothedRTT = (mSmoothedRTT * (kRTTAlphaDenominator - 1) + aRTT) / kRTTAlphaDenominator;
}

Sequence Socket::GetSndUna(void) const
{
    return GetSendQueue().GetStartSeq();
}

Sequence Socket::GetSndNxt(void) const
{
    return GetSendQueue().GetSendNextSeq();
}

void Socket::RequireAckPeer(void)
{
    mPendingFlags |= kPendingFlagRequireAckPeer;
    ResetTimer();
}

void Socket::TriggerEvent(otTcpSocketEvent aEvent)
{
    OT_ASSERT(mEventHandler != nullptr);

    mEventHandler(reinterpret_cast<otTcpSocket *>(this), aEvent);
}

void Socket::ResetTimer()
{
    TimeMilli now = TimerMilli::GetNow();
    TimeMilli nextEventTime;

    //
    // Factors that impact the Timer:
    // 1. Whether or not the state is `OT_TCP_STATE_TIME_WAIT`
    // 2. The `mPendingFlags` flags
    // 3. The Send Queue (the next segment to send)
    // 4. TCP status: RTT, SND.WND
    //

    if (mPendingFlags != 0)
    {
        nextEventTime = now;
    }
    else if (mState == OT_TCP_STATE_TIME_WAIT)
    {
        nextEventTime = mTimeWaitStartTime + static_cast<uint32_t>(kMaxSegmentLifetime) * 2;
    }
    else
    {
        nextEventTime = GetSendQueue().GetNextSendTime(now, GetRTT());
    }

    if (nextEventTime == now.GetDistantFuture())
    {
        Get<Tcp>().StopTimer(mTimer);
        otLogDebgTcp("Timer stopped");
    }
    else
    {
        if (nextEventTime < now + 1000 && Get<MessagePool>().GetFreeBufferCount() <= kMinFreeMessageBufferThreshold)
        {
            // if we don't have message buffers, the timer should delay for at least 1s
            nextEventTime = now + 1000;
        }

        Get<Tcp>().StartTimer(mTimer, nextEventTime);
        otLogDebgTcp("Timer will fire after %ums, free buffers: %d", nextEventTime - now,
                     Get<MessagePool>().GetFreeBufferCount());
    }
}

void Socket::LogTcp(const char *aAction, const Message &aMessage, const Header &aHeader)
{
#if OPENTHREAD_CONFIG_LOG_TCP && OPENTHREAD_CONFIG_LOG_LEVEL >= OT_LOG_LEVEL_DEBG
    uint16_t segLen = Tcp::GetSegmentLength(aHeader, aMessage);

    otLogDebgTcp("%s: %d->%d,SEQ=%d,LEN=%d,ACK=%d,[%s|%s|%s|%s|%s|%s],WIN=%d", aAction, aHeader.GetSourcePort(),
                 aHeader.GetDestinationPort(), aHeader.GetSequenceNumber(), segLen, aHeader.GetAcknowledgmentNumber(),
                 aHeader.GetFlags() & kFlagSYN ? "SYN" : "", aHeader.GetFlags() & kFlagFIN ? "FIN" : "",
                 aHeader.GetFlags() & kFlagRST ? "RST" : "", aHeader.GetFlags() & kFlagACK ? "ACK" : "",
                 aHeader.GetFlags() & kFlagPSH ? "PSH" : "", aHeader.GetFlags() & kFlagURG ? "URG" : "",
                 aHeader.GetWindow());
#else
    OT_UNUSED_VARIABLE(aAction);
    OT_UNUSED_VARIABLE(aMessage);
    OT_UNUSED_VARIABLE(aHeader);
#endif
}

void Socket::NotifyDataReceived(void)
{
    uint32_t readable;

    readable = GetRecvQueue().GetReadable();
    otLogDebgTcp("TCP readable: %u", readable);

    if (readable > 0)
    {
        mPendingFlags |= kPendingFlagNotifyDataReceived;
        ResetTimer();
    }
}

void Socket::NotifyDataSent(void)
{
    if (CanSendData())
    {
        mPendingFlags |= kPendingFlagNotifyDataSent;
        ResetTimer();
    }
}

uint16_t Socket::Read(uint8_t *aBuf, uint16_t aSize)
{
    return GetRecvQueue().Read(aBuf, aSize);
}

void Tcp::HandleMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError               error = OT_ERROR_NONE;
    Header                tcpHeader;
    Socket::SegmentAction action = Socket::kActionReset;

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    VerifyOrExit(Random::NonCrypto::GetUint8InRange(0, 100) >= mSegmentRandomDropProb, error = OT_ERROR_DROP);
#endif

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    SuccessOrExit(error = Checksum::VerifyMessageChecksum(aMessage, aMessageInfo, kProtoTcp));
#endif

    aMessage.RemoveHeader(aMessage.GetOffset());
    SuccessOrExit(error = aMessage.Read(0, tcpHeader));

    aMessage.MoveOffset(tcpHeader.GetHeaderSize());

    aMessageInfo.mPeerPort = tcpHeader.GetSourcePort();
    aMessageInfo.mSockPort = tcpHeader.GetDestinationPort();

    otLogDebgTcp("Received TCP message: %s:%d -> %s:%d", aMessageInfo.GetPeerAddr().ToString().AsCString(),
                 aMessageInfo.GetPeerPort(), aMessageInfo.GetSockAddr().ToString().AsCString(),
                 aMessageInfo.GetSockPort());

    otLogDebgTcp("TCP <<< %d->%d,SEQ=%d,LEN=%d,ACK=%d,[%s|%s|%s|%s|%s|%s],WIN=%d", tcpHeader.GetSourcePort(),
                 tcpHeader.GetDestinationPort(), tcpHeader.GetSequenceNumber(),
                 aMessage.GetLength() - aMessage.GetOffset(), tcpHeader.GetAcknowledgmentNumber(),
                 tcpHeader.GetFlags() & kFlagSYN ? "SYN" : "", tcpHeader.GetFlags() & kFlagFIN ? "FIN" : "",
                 tcpHeader.GetFlags() & kFlagRST ? "RST" : "", tcpHeader.GetFlags() & kFlagACK ? "ACK" : "",
                 tcpHeader.GetFlags() & kFlagPSH ? "PSH" : "", tcpHeader.GetFlags() & kFlagURG ? "URG" : "",
                 tcpHeader.GetWindow());

    for (Socket *sock = mSockets.GetHead(); sock != nullptr; sock = sock->GetNext())
    {
        if (!ShouldHandleTcpMessage(*sock, tcpHeader, aMessageInfo))
        {
            continue;
        }

        sock->HandleMessage(tcpHeader, aMessage, aMessageInfo, action);

        switch (action)
        {
        case Socket::kActionNone:
            break;

        case Socket::kActionAck:
            if (!tcpHeader.HasFlags(kFlagRST))
            {
                sock->RequireAckPeer();
            }
            break;

        case Socket::kActionReset:
            if (!tcpHeader.HasFlags(kFlagRST))
            {
                RespondReset(tcpHeader, aMessage, aMessageInfo, sock);
            }
            break;

        case Socket::kActionAbort:
            sock->OnAborted();
            break;

        case Socket::kActionReceive:
            sock->ProcessRecvQueue(aMessageInfo);
            break;
        }

        // always reset the Timer after a segment is processed because many factors could impact the Timer
        sock->ResetTimer();
        ExitNow();
    }

    error = OT_ERROR_NOT_FOUND;

    if (!tcpHeader.HasFlags(kFlagRST))
    {
        RespondReset(tcpHeader, aMessage, aMessageInfo, nullptr);
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        aMessage.Free();
        otLogWarnTcp("Failed to process TCP message: %s", otThreadErrorToString(error));
    }
}

void Socket::HandleMessage(const Header &     aHeader,
                           Message &          aMessage,
                           const MessageInfo &aMessageInfo,
                           SegmentAction &    aAction)
{
    uint16_t segmentSize     = Tcp::GetSegmentLength(aHeader, aMessage);
    bool     messageReceived = false;

    otLogDebgTcp("  TCP state: %s", StateToString(mState));

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    if (mResetNextSegment)
    {
        mResetNextSegment = false;
        ExitNow(aAction = kActionReset);
    }
#endif

    aAction = kActionAck;

    switch (mState)
    {
    case OT_TCP_STATE_CLOSED:
        CHECK_TCP_INVARIANT(false);
        break;
    case OT_TCP_STATE_LISTEN:
        VerifyOrExit(!aHeader.HasFlags(kFlagRST), aAction = kActionNone);
        VerifyOrExit(!aHeader.HasFlags(kFlagACK), aAction = kActionReset);
        VerifyOrExit(aHeader.HasFlags(kFlagSYN), aAction = kActionReset);
        VerifyOrExit(!aHeader.HasFlags(kFlagFIN), aAction = kActionNone);

        CHECK_TCP_INVARIANT(GetPeerName().GetAddress().IsUnspecified());
        CHECK_TCP_INVARIANT(GetPeerName().mPort == 0);

        GetRecvQueue().Init(aHeader.GetSequenceNumber() + 1);

        GetPeerName().GetAddress() = aMessageInfo.GetPeerAddr();
        GetPeerName().mPort        = aHeader.GetSourcePort();

        SendSYN();

        break;

    case OT_TCP_STATE_SYN_SENT:
        // TODO: handle message without ACK
        VerifyOrExit(aHeader.HasFlags(kFlagACK), otLogWarnTcp("not has ACK"); aAction = kActionReset);
        VerifyOrExit(IsAckAcceptable(aHeader.GetAcknowledgmentNumber()),
                     otLogWarnTcp("ACK is not acceptable: %d <= %d < %d", GetSndUna(),
                                  aHeader.GetAcknowledgmentNumber(), GetSndNxt());
                     aAction = kActionReset);

        VerifyOrExit(!aHeader.HasFlags(kFlagRST), aAction = kActionAbort);
        VerifyOrExit(aHeader.HasFlags(kFlagSYN), otLogWarnTcp("not has SYN"); aAction = kActionNone);
        VerifyOrExit(!aHeader.HasFlags(kFlagFIN), aAction = kActionNone);

        // RCV.NXT is set to SEG.SEQ+1, IRS is set to SEG.SEQ
        GetRecvQueue().Init(aHeader.GetSequenceNumber() + 1);
        GetSendQueue().ConfigSendWindowBySYN(aHeader);
        SetState(OT_TCP_STATE_ESTABLISHED);

        ProcessAck(aHeader);

        break;

    case OT_TCP_STATE_SYN_RCVD:
    case OT_TCP_STATE_ESTABLISHED:
    case OT_TCP_STATE_FIN_WAIT_1:
    case OT_TCP_STATE_FIN_WAIT_2:
    case OT_TCP_STATE_CLOSE_WAIT:
    case OT_TCP_STATE_LAST_ACK:
    case OT_TCP_STATE_CLOSING:
    case OT_TCP_STATE_TIME_WAIT:
    {
        bool    isDuplicate = false;
        otError error;

        if (!IsSeqAcceptable(aHeader, segmentSize, isDuplicate))
        {
            otLogWarnTcp("not acceptable SEQ: %u <= %u+%d <= %u+%d, duplicate=%s", GetRcvNxt(),
                         aHeader.GetSequenceNumber(), segmentSize, GetRcvNxt(), GetReceiveWindowSize(),
                         isDuplicate ? "Y" : "N");

            VerifyOrExit(!isDuplicate, aAction = kActionAck);
        }

        if ((error = GetRecvQueue().Add(aMessage)) != OT_ERROR_NONE)
        {
            otLogWarnTcp("not acceptable SEQ: %u <= %u+%d <= %u+%d: %s", GetRcvNxt(), aHeader.GetSequenceNumber(),
                         segmentSize, GetRcvNxt(), GetReceiveWindowSize(), otThreadErrorToString(error));
            ExitNow(aAction = kActionAck);
        }

        messageReceived = true;
        ExitNow(aAction = kActionReceive);
    }
    break;
    }

exit:
    if (!messageReceived)
    {
        aMessage.Free();
    }
}

void Socket::ProcessRecvQueue(const MessageInfo &aMessageInfo)
{
    while (mState != OT_TCP_STATE_CLOSED)
    {
        Message *message = GetRecvQueue().Process();

        VerifyOrExit(message != nullptr);
        ProcessReceivedSegment(*message, aMessageInfo);
    }

exit:
    if (!GetRecvQueue().IsProcessEmpty())
    {
        // a gap is found, so tell the peer about our RCV.NXT
        RequireAckPeer();
    }

    GetRecvQueue().ClearEmptySegments();
    NotifyDataReceived();
}

void Socket::ProcessReceivedSegment(Message &aMessage, const MessageInfo &aMessageInfo)
{
    Header        header;
    SegmentAction action = kActionAck;
    uint16_t      segmentSize;

    IgnoreError(aMessage.Read(0, header));
    segmentSize = Tcp::GetSegmentLength(header, aMessage);

    LogTcp("ProcessReceivedSegment", aMessage, header);

    // check the RST bit
    VerifyOrExit(!header.HasFlags(kFlagRST), otLogWarnTcp("unexpected RST"); action = kActionAbort);
    // check the SYN bit
    VerifyOrExit(!header.HasFlags(kFlagSYN), otLogWarnTcp("unexpected SYN"); action = kActionReset);
    // check the ACK field
    VerifyOrExit(header.HasFlags(kFlagACK), otLogWarnTcp("expected ACK"); action = kActionNone);

    if (mState == OT_TCP_STATE_SYN_RCVD)
    {
        VerifyOrExit(IsAckAcceptable(header.GetAcknowledgmentNumber()), action = kActionReset);

        SetState(OT_TCP_STATE_ESTABLISHED);
    }

    VerifyOrExit(header.GetAcknowledgmentNumber() >= GetSndUna(),
                 otLogWarnTcp("duplicate!acked before!SND.UNA=%d", GetSndUna());
                 action = kActionNone);
    VerifyOrExit(header.GetAcknowledgmentNumber() <= GetSndNxt(), otLogWarnTcp("ACK unsent!"); action = kActionAck);

    ProcessAck(header);

    ProcessFIN(header);

    VerifyOrExit(segmentSize > 0, action = kActionNone);

exit:

    otLogDebgTcp("ProcessReceivedSegment returns %d, segmentSize=%d", action, segmentSize);

    switch (action)
    {
    case kActionNone:
        break;
    case kActionAck:
        RequireAckPeer();
        break;
    case kActionReset:
        Get<Tcp>().RespondReset(header, aMessage, aMessageInfo, this);
        break;
    case kActionAbort:
        OnAborted();
        break;
    case kActionReceive:
        OT_ASSERT(false);
        break;
    }
}

void Tcp::TakeCustody(Message &aMessage)
{
    bool notified = false;

    for (Socket *sock = mSockets.GetHead(); sock != nullptr; sock = sock->GetNext())
    {
        if (sock->TakeCustody(aMessage))
        {
            notified = true;
            break;
        }
    }

    if (!notified)
    {
        // the message might have been ACKed
        aMessage.SetIsManagedByTcp(false);
        aMessage.Free();
    }
}

void Socket::Send(void)
{
    Message *   message = nullptr;
    otError     error   = OT_ERROR_NONE;
    Header      tcpHeader;
    MessageInfo messageInfo;
    uint8_t     flags = 0;
    Sequence    seq;
    bool        isNewMessage = false;

    CHECK_TCP_INVARIANT(mState != OT_TCP_STATE_CLOSED && mState != OT_TCP_STATE_LISTEN);

    if (mState == OT_TCP_STATE_SYN_SENT)
    {
        CHECK_TCP_INVARIANT(!GetPeerName().GetAddress().IsUnspecified());
        CHECK_TCP_INVARIANT(GetPeerName().mPort != 0);
    }
    else
    {
        flags |= kFlagACK;
    }

    message = GetSendQueue().GetSendNext(GetRTT(), seq, flags);

    if (message == nullptr)
    {
        message = Get<Tcp>().NewMessage();
        VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);
        isNewMessage = true;
    }

    tcpHeader.SetSourcePort(GetSockName().mPort);
    tcpHeader.SetDestinationPort(GetPeerName().mPort);
    tcpHeader.SetHeaderSize(sizeof(tcpHeader));
    tcpHeader.SetAcknowledgmentNumber(GetRcvNxt());
    tcpHeader.SetWindow(GetReceiveWindowSize());
    tcpHeader.SetChecksum(0);
    tcpHeader.SetUrgentPointer(0);
    tcpHeader.SetSequenceNumber(seq);
    tcpHeader.SetFlags(flags);

    messageInfo.SetSockAddr(GetSockName().GetAddress());
    messageInfo.SetSockPort(GetSockName().mPort);
    messageInfo.SetPeerAddr(GetPeerName().GetAddress());
    messageInfo.SetPeerPort(GetPeerName().mPort);

    if (flags & kFlagSYN)
    {
        SuccessOrExit(error = AddMaxSegmentSizeOption(tcpHeader, *message));
    }

    message->Write(0, tcpHeader);

    SuccessOrExit(error = Get<Tcp>().SendMessage(*message, messageInfo));

    if (flags & kFlagACK)
    {
        mPendingFlags &= ~kPendingFlagRequireAckPeer;
    }

exit:
    otLogDebgTcp("TCP:%d - Send: %s", mSockName.mPort, otThreadErrorToString(error));

    if (isNewMessage && error != OT_ERROR_NONE)
    {
        FreeMessage(message);
    }
}

otError Tcp::SendMessage(Message &aMessage, MessageInfo &aMessageInfo)
{
    otError error = OT_ERROR_NONE;

    aMessage.SetOffset(0);

    error = Get<Ip6>().SendDatagram(aMessage, aMessageInfo, kProtoTcp);
    return error;
}

bool Tcp::ShouldHandleTcpMessage(const Socket &aSocket, const Header &aHeader, const MessageInfo &aMessageInfo)
{
    bool handle = false;

    VerifyOrExit(aSocket.mState != OT_TCP_STATE_CLOSED);

    otLogDebgTcp("Checking TCP socket: %s,%d<-%s,%d for message %s,%d<-%s,%d",
                 aSocket.GetSockName().GetAddress().ToString().AsCString(), aSocket.GetSockName().mPort,
                 aSocket.GetPeerName().GetAddress().ToString().AsCString(), aSocket.GetPeerName().mPort,
                 aMessageInfo.GetSockAddr().ToString().AsCString(), aHeader.GetDestinationPort(),
                 aMessageInfo.GetPeerAddr().ToString().AsCString(), aHeader.GetSourcePort());

    VerifyOrExit(aSocket.GetSockName().mPort == aHeader.GetDestinationPort());
    VerifyOrExit(aSocket.GetSockName().GetAddress().IsUnspecified() ||
                 aSocket.GetSockName().GetAddress() == aMessageInfo.GetSockAddr());

    VerifyOrExit(aSocket.GetPeerName().mPort == 0 || aSocket.GetPeerName().mPort == aHeader.GetSourcePort());
    VerifyOrExit(aSocket.GetPeerName().GetAddress().IsUnspecified() ||
                 aSocket.GetPeerName().GetAddress() == aMessageInfo.GetPeerAddr());

    handle = true;

exit:
    return handle;
}

const char *Tcp::StateToString(otTcpState aState)
{
    const char *string = "Invalid";

    switch (aState)
    {
    case OT_TCP_STATE_CLOSED:
        string = "CLOSED";
        break;
    case OT_TCP_STATE_LISTEN:
        string = "LISTEN";
        break;
    case OT_TCP_STATE_SYN_SENT:
        string = "SYN-SENT";
        break;
    case OT_TCP_STATE_SYN_RCVD:
        string = "SYN-RECEIVED";
        break;
    case OT_TCP_STATE_ESTABLISHED:
        string = "ESTABLISHED";
        break;
    case OT_TCP_STATE_FIN_WAIT_1:
        string = "FIN-WAIT-1";
        break;
    case OT_TCP_STATE_FIN_WAIT_2:
        string = "FIN-WAIT-2";
        break;
    case OT_TCP_STATE_CLOSE_WAIT:
        string = "CLOSE-WAIT";
        break;
    case OT_TCP_STATE_LAST_ACK:
        string = "LAST-ACK";
        break;
    case OT_TCP_STATE_CLOSING:
        string = "CLOSING";
        break;
    case OT_TCP_STATE_TIME_WAIT:
        string = "TIME-WAIT";
        break;
    }

    return string;
}

void Tcp::RespondReset(const Header &     aTcpHeader,
                       const Message &    aMessage,
                       const MessageInfo &aMessageInfo,
                       const Socket *     aSocket)
{
    bool receiveAck = aTcpHeader.HasFlags(kFlagACK);

    SendReset(aSocket != nullptr ? aSocket->GetSockName().GetAddress() : aMessageInfo.GetSockAddr(),
              aTcpHeader.GetDestinationPort(),
              aSocket != nullptr ? aSocket->GetPeerName().GetAddress() : aMessageInfo.GetPeerAddr(),
              aTcpHeader.GetSourcePort(), !receiveAck, receiveAck ? aTcpHeader.GetAcknowledgmentNumber() : Sequence(0),
              receiveAck ? Sequence(0) : aTcpHeader.GetSequenceNumber() + Tcp::GetSegmentLength(aTcpHeader, aMessage));
}

void Tcp::SendReset(const Address &aSrcAddr,
                    uint16_t       aSrcPort,
                    const Address &aDstAddr,
                    uint16_t       aDstPort,
                    bool           aSetAck,
                    Sequence       aSeq,
                    Sequence       aAckNumber)
{
    otError     error   = OT_ERROR_NONE;
    Message *   message = nullptr;
    Header      tcpHeader;
    MessageInfo messageInfo;

    message = NewMessage();
    VerifyOrExit(message != nullptr, error = OT_ERROR_NO_BUFS);

    tcpHeader.SetSourcePort(aSrcPort);
    tcpHeader.SetDestinationPort(aDstPort);
    tcpHeader.SetHeaderSize(sizeof(tcpHeader));
    tcpHeader.SetAcknowledgmentNumber(aAckNumber);
    tcpHeader.SetWindow(0);
    tcpHeader.SetChecksum(0);
    tcpHeader.SetUrgentPointer(0);
    tcpHeader.SetSequenceNumber(aSeq);
    tcpHeader.SetFlags(aSetAck ? (kFlagRST | kFlagACK) : kFlagRST);

    messageInfo.SetSockAddr(aSrcAddr);
    messageInfo.SetSockPort(aSrcPort);
    messageInfo.SetPeerAddr(aDstAddr);
    messageInfo.SetPeerPort(aDstPort);

    message->Write(0, tcpHeader);

    SuccessOrExit(error = SendMessage(*message, messageInfo));

exit:
    FreeMessageOnError(message, error);
    if (error != OT_ERROR_NONE)
    {
        otLogWarnTcp("Send RST failed: %s", otThreadErrorToString(error));
    }
}

uint16_t Tcp::GetSegmentLength(const Tcp::Header &aHeader, const Message &aMessage)
{
    return aMessage.GetLength() - aMessage.GetOffset() + (aHeader.HasFlags(kFlagSYN) ? 1 : 0) +
           (aHeader.HasFlags(kFlagFIN) ? 1 : 0);
}

Socket::Socket(Instance &aInstance, otTcpEventHandler aEventHandler, void *aContext)
    : InstanceLocator(aInstance)
    , mEventHandler(aEventHandler)
    , mContext(aContext)
    , mTimer(*this)
    , mTimeWaitStartTime(0)
    , mSmoothedRTT(kInitialRTT)
    , mMinRoundTripTime(kDefaultMinRoundTripTime)
    , mMaxRoundTripTime(kDefaultMaxRoundTripTime)
    , mState(OT_TCP_STATE_CLOSED)
    , mPendingFlags(0)
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
    , mResetNextSegment(false)
#endif

{
    OT_ASSERT(mEventHandler != nullptr);
    Get<Tcp>().AddSocket(*this);
}

Socket::~Socket(void)
{
    Get<Tcp>().RemoveSocket(*this);
}

otError Socket::Bind(const SockAddr &aAddr)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aAddr.GetAddress().IsUnspecified() || Get<ThreadNetif>().HasUnicastAddress(aAddr.GetAddress()),
                 error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(!IsBound(), error = OT_ERROR_ALREADY);

    mSockName = aAddr;

    if (!IsBound())
    {
        mSockName.mPort = Get<Tcp>().GetEphemeralPort();
    }

exit:
    return error;
}

otError Socket::Connect(const SockAddr &aAddr)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == OT_TCP_STATE_CLOSED, error = OT_ERROR_INVALID_STATE);

    mPeerName = aAddr;

    if (!IsBound())
    {
        SuccessOrExit(error = Bind(GetSockName()));
    }

    SendSYN();

exit:
    return error;
}

void Socket::HandleTimer(void)
{
    otLogDebgTcp("TCP timer fired");

    if (mPendingFlags & (kPendingFlagNotifyDataReceived | kPendingFlagNotifyDataSent))
    {
        if (mPendingFlags & kPendingFlagNotifyDataReceived)
        {
            mPendingFlags &= ~kPendingFlagNotifyDataReceived;
            TriggerEvent(OT_TCP_SOCKET_DATA_RECEIVED);
        }

        if (mPendingFlags & kPendingFlagNotifyDataSent)
        {
            mPendingFlags &= ~kPendingFlagNotifyDataSent;

            if (CanSendData())
            {
                TriggerEvent(OT_TCP_SOCKET_DATA_SENT);
            }
        }

        ExitNow();
    }
    if (mState == OT_TCP_STATE_TIME_WAIT)
    {
        TimeMilli now = TimerMilli::GetNow();

        if (now - mTimeWaitStartTime >= kMaxSegmentLifetime * 2)
        {
            SetState(OT_TCP_STATE_CLOSED);
            ExitNow();
        }
    }

    Send();

exit:
    ResetTimer();
}

uint16_t Socket::Write(const uint8_t *aData, uint16_t aLength)
{
    SendWindow &sendQueue = GetSendQueue();
    uint16_t    payloadSize;
    uint16_t    sentLength = 0;

    VerifyOrExit(CanSendData());

    while (aLength > 0)
    {
        bool     isNewMessage = false;
        Message *message      = sendQueue.GetWritableMessage();
        uint16_t segLen;

        if (message == nullptr)
        {
            VerifyOrExit(!sendQueue.IsFull(), otLogWarnTcp("SND.QUE is full!"));
            VerifyOrExit(sendQueue.GetStopSeq() <= sendQueue.GetStartSeq() + sendQueue.GetSendWindowSize(),
                         otLogWarnTcp("SND.WND is full!"));

            message = Get<Tcp>().NewMessage();
            VerifyOrExit(message != nullptr);
            isNewMessage = true;
        }

        payloadSize = message->GetLength() - message->GetOffset();
        CHECK_TCP_INVARIANT(payloadSize < kMaxSegmentSize);
        segLen = OT_MIN(aLength, kMaxSegmentSize - payloadSize);

        SuccessOrExit(message->AppendBytes(aData, segLen));

        if (isNewMessage)
        {
            sendQueue.Add(*message);
        }

        aData += segLen;
        aLength -= segLen;
        sentLength += segLen;
    }

exit:
    if (sentLength)
    {
        ResetTimer();
    }

    return sentLength;
}

void Socket::SendSYN(void)
{
    CHECK_TCP_INVARIANT(mState == OT_TCP_STATE_LISTEN || mState == OT_TCP_STATE_CLOSED);

    GetSendQueue().AddSYN();
    SetState(mState == OT_TCP_STATE_LISTEN ? OT_TCP_STATE_SYN_RCVD : OT_TCP_STATE_SYN_SENT);
}

bool Socket::TakeCustody(Message &aMessage)
{
    return GetSendQueue().TakeCustody(aMessage);
}

Sequence Socket::GetRcvNxt(void)
{
    CHECK_TCP_INVARIANT(mState != OT_TCP_STATE_CLOSED);
    CHECK_TCP_INVARIANT(mState != OT_TCP_STATE_LISTEN);

    return GetRecvQueue().GetStartSeq();
}

void Socket::Close(void)
{
    switch (mState)
    {
    case OT_TCP_STATE_LISTEN:
    case OT_TCP_STATE_SYN_SENT:
        CHECK_TCP_INVARIANT(GetSendQueue().IsEmpty());
        CHECK_TCP_INVARIANT(GetRecvQueue().IsEmpty());
        SetState(OT_TCP_STATE_CLOSED);
        break;
    case OT_TCP_STATE_SYN_RCVD:
        CHECK_TCP_INVARIANT(GetSendQueue().IsEmpty());
        CHECK_TCP_INVARIANT(GetRecvQueue().IsEmpty());
        // fallthrough
    case OT_TCP_STATE_ESTABLISHED:
    case OT_TCP_STATE_CLOSE_WAIT:
        SendFIN();
        break;
    default:
        // connection is already closing or closed
        break;
    }
}

void Socket::ProcessFIN(const Header &aHeader)
{
    VerifyOrExit(aHeader.HasFlags(kFlagFIN));

    switch (mState)
    {
    case OT_TCP_STATE_ESTABLISHED:
        SetState(OT_TCP_STATE_CLOSE_WAIT);
        break;
    case OT_TCP_STATE_FIN_WAIT_1:
        SetState(OT_TCP_STATE_CLOSING);
        break;
    case OT_TCP_STATE_FIN_WAIT_2:
        SetState(OT_TCP_STATE_TIME_WAIT);
        break;
    case OT_TCP_STATE_CLOSE_WAIT:
    case OT_TCP_STATE_LAST_ACK:
    case OT_TCP_STATE_CLOSING:
        break;
    case OT_TCP_STATE_TIME_WAIT:
        break;
    default:
        CHECK_TCP_INVARIANT(false);
    }
exit:
    return;
}

bool Socket::HasFINToBeAcked() const
{
    return mState == OT_TCP_STATE_FIN_WAIT_1 || mState == OT_TCP_STATE_LAST_ACK || mState == OT_TCP_STATE_CLOSING;
}

void Socket::SendFIN(void)
{
    CHECK_TCP_INVARIANT(mState == OT_TCP_STATE_SYN_RCVD || mState == OT_TCP_STATE_ESTABLISHED ||
                        mState == OT_TCP_STATE_CLOSE_WAIT);

    GetSendQueue().AddFIN();

    SetState(mState == OT_TCP_STATE_CLOSE_WAIT ? OT_TCP_STATE_LAST_ACK : OT_TCP_STATE_FIN_WAIT_1);
}

bool Socket::CanSendData(void)
{
    return mState == OT_TCP_STATE_ESTABLISHED || mState == OT_TCP_STATE_CLOSE_WAIT;
}

void Socket::ProcessFINAcked(void)
{
    VerifyOrExit(HasFINToBeAcked());
    VerifyOrExit(GetSendQueue().IsEmpty());

    switch (mState)
    {
    case OT_TCP_STATE_FIN_WAIT_1:
        SetState(OT_TCP_STATE_FIN_WAIT_2);
        break;
    case OT_TCP_STATE_CLOSING:
        SetState(OT_TCP_STATE_TIME_WAIT);
        break;
    case OT_TCP_STATE_LAST_ACK:
        SetState(OT_TCP_STATE_CLOSED);
        break;
    default:
        CHECK_TCP_INVARIANT(false);
    }

exit:
    return;
}

otError Socket::Listen(void)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(mState == OT_TCP_STATE_CLOSED, error = OT_ERROR_INVALID_STATE);
    VerifyOrExit(IsBound(), error = OT_ERROR_INVALID_STATE);

    SetState(OT_TCP_STATE_LISTEN);
exit:
    return error;
}

void Socket::OnAborted(void)
{
    CHECK_TCP_INVARIANT(mState != OT_TCP_STATE_CLOSED);

    GetSendQueue().Flush();
    GetRecvQueue().Flush();

    TriggerEvent(OT_TCP_SOCKET_ABORTED);

    SetState(OT_TCP_STATE_CLOSED);
}

void Socket::Abort(void)
{
    switch (mState)
    {
    case OT_TCP_STATE_SYN_RCVD:
    case OT_TCP_STATE_ESTABLISHED:
    case OT_TCP_STATE_FIN_WAIT_1:
    case OT_TCP_STATE_FIN_WAIT_2:
    case OT_TCP_STATE_CLOSE_WAIT:
        Get<Tcp>().SendReset(static_cast<Address &>(mSockName.mAddress), mSockName.mPort,
                             static_cast<Address &>(mPeerName.mAddress), mPeerName.mPort, false, GetSndNxt(),
                             Sequence(0));
        // fallthrough
    case OT_TCP_STATE_LISTEN:
    case OT_TCP_STATE_SYN_SENT:
    case OT_TCP_STATE_LAST_ACK:
    case OT_TCP_STATE_CLOSING:
    case OT_TCP_STATE_TIME_WAIT:
        OnAborted();
        break;

    case OT_TCP_STATE_CLOSED:
        break;
    }

    OT_ASSERT(mState == OT_TCP_STATE_CLOSED);
}

otError Socket::AddMaxSegmentSizeOption(Header &aHeader, Message &aMessage)
{
    otError error = OT_ERROR_NONE;

    CHECK_TCP_INVARIANT(aMessage.GetOffset() == sizeof(Header));

    SuccessOrExit(error = aMessage.Append(kOptionKindMaxSegmentSize));
    SuccessOrExit(error = aMessage.Append(kMaxSegmentSizeOptionSize));
    SuccessOrExit(error = aMessage.Append(HostSwap16(kMaxSegmentSize)));

    static_assert((sizeof(Header) + kMaxSegmentSizeOptionSize) % 4 == 0, "TCP header size must be a multiple of 4B.");
    aHeader.SetHeaderSize(sizeof(Header) + kMaxSegmentSizeOptionSize);

exit:
    return error;
}

otError Socket::ConfigRoundTripTime(uint32_t aMinRTT, uint32_t aMaxRTT)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit(aMinRTT <= aMaxRTT, error = OT_ERROR_INVALID_ARGS);

    mMinRoundTripTime = aMinRTT;
    mMaxRoundTripTime = aMaxRTT;
exit:
    return error;
}

Socket::SendWindow::SendWindow(void)
    : mStartSeq(GenerateInitialSendSequence())
    , mSndWl1(0)
    , mSndWl2(0)
    , mSndWnd(1) // Initialize SND.WND to 1 to allow send SYN, will be override by SEG.WND from peer
    , mStartIndex(0)
    , mLength(0)
    , mPendingFIN(false)
{
}

Sequence Socket::SendWindow::GenerateInitialSendSequence(void)
{
    //    return Sequence(Random::NonCrypto::GetUint32());
    return Sequence(0);
}

uint8_t Socket::SendWindow::ReclaimAcked(Sequence aAckNumber, uint32_t &aRTT)
{
    Sequence seq          = mStartSeq;
    uint8_t  reclaimCount = 0;

    for (uint8_t i = 0; i < mLength; i++)
    {
        Entry &entry = GetEntry(i);

        VerifyOrExit(aAckNumber >= seq + entry.GetSegmentLength());

        reclaimCount++;
        seq += entry.GetSegmentLength();
    }

exit:

    otLogDebgTcp("ProcessAck: ACK=%u, reclaim count=%d, left=%d", aAckNumber.GetValue(), reclaimCount,
                 mLength - reclaimCount);

    for (uint8_t i = 0; i < reclaimCount; i++)
    {
        uint32_t rtt;

        ReclaimHead(rtt);

        if (i == 0 || aRTT > rtt)
        {
            aRTT = rtt;
        }
    }

    CheckInvariant();
    return reclaimCount;
}

Message *Socket::SendWindow::GetSendNext(uint32_t aRTT, Sequence &aSeq, uint8_t &aFlags)
{
    Message * sendNext = nullptr;
    TimeMilli now;

    aSeq = mStartSeq;

    VerifyOrExit(mLength > 0);

    now = TimerMilli::GetNow();

    for (uint8_t i = 0; i < mLength; i++)
    {
        Entry &   entry = GetEntry(i);
        TimeMilli nextSendTime;
        Message * message = entry.mMessage;

        if (message == nullptr || !message->IsInAQueue())
        {
            nextSendTime = entry.mLastSendTime + GetSendTimeout(entry, aSeq, aRTT);

            // ignore kNewMessageSendTimeout if the send time is close enough
            if (now + kNewMessageSendTimeout >= nextSendTime)
            {
                entry.mSendCount++;
                entry.mLastSendTime = now;

                if (entry.mIsSyn)
                {
                    aFlags |= kFlagSYN;
                }

                if (entry.mIsFin)
                {
                    aFlags |= kFlagFIN;
                }

                ExitNow(sendNext = message);
            }
        }

        aSeq += entry.GetSegmentLength();
    }

exit:
    otLogDebgTcp("GetSendNext Seq=%lu, StartSeq=%lu", aSeq.GetValue(), mStartSeq.GetValue());
    return sendNext;
}

void Socket::SendWindow::CheckInvariant(void)
{
#if OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
    CHECK_TCP_INVARIANT(mStartIndex < OT_ARRAY_LENGTH(mSegments));
    CHECK_TCP_INVARIANT(mLength <= OT_ARRAY_LENGTH(mSegments));

    bool foundNotSend = false;

    for (uint8_t i = 0; i < mLength; i++)
    {
        Entry &entry = GetEntry(i);

        CHECK_TCP_INVARIANT(!(entry.mIsSyn && entry.mIsFin));
        CHECK_TCP_INVARIANT(!entry.mIsSyn || entry.mMessage == nullptr);
        CHECK_TCP_INVARIANT(entry.mIsSyn || entry.mIsFin || entry.mMessage != nullptr);

        CHECK_TCP_INVARIANT(!foundNotSend || entry.mSendCount == 0);
        if (entry.mSendCount == 0)
        {
            foundNotSend = true;
        }
    }

    for (uint8_t i = mLength; i < OT_ARRAY_LENGTH(mSegments); i++)
    {
        Entry &entry = GetEntry(i);

        OT_UNUSED_VARIABLE(entry);
        CHECK_TCP_INVARIANT(entry.mMessage == nullptr);
    }
#endif
}

Message *Socket::SendWindow::GetWritableMessage(void)
{
    Message *writableMsg = nullptr;

    VerifyOrExit(mLength > 0);

    {
        Entry &entry = GetEntry(mLength - 1);

        VerifyOrExit(entry.IsWritable());
        writableMsg = entry.mMessage;
    }
exit:
    return writableMsg;
}

bool Socket::SendWindow::IsFull(void)
{
    return mLength == OT_ARRAY_LENGTH(mSegments);
}

Sequence Socket::SendWindow::GetStopSeq(void) const
{
    Sequence seq = mStartSeq;

    for (uint8_t i = 0; i < mLength; i++)
    {
        seq += GetEntry(i).GetSegmentLength();
    }

    return seq;
}

Sequence Socket::SendWindow::GetSendNextSeq(void) const
{
    Sequence seq = mStartSeq;

    for (uint8_t i = 0; i < mLength; i++)
    {
        const Entry &entry = GetEntry(i);

        VerifyOrExit(entry.mSendCount > 0);
        seq += entry.GetSegmentLength();
    }

exit:
    return seq;
}

void Socket::SendWindow::ReclaimHead(uint32_t &aRTT)
{
    OT_ASSERT(mLength > 0);

    Entry &entry = GetEntry(0);

    aRTT = TimerMilli::GetNow() - entry.mLastSendTime;

    otLogDebgTcp("ReclaimHead: SEG.LEN=%d, Seq=%u, LST=%u, SC=%d, RTT=%u", entry.GetSegmentLength(),
                 mStartSeq.GetValue(), entry.mLastSendTime, entry.mSendCount, aRTT);

    mStartSeq += entry.GetSegmentLength();

    FreeEntry(entry);

    mStartIndex = (mStartIndex + 1) % kMaxSendSegments;
    mLength--;

    if (mPendingFIN)
    {
        mPendingFIN = false;
        Add(nullptr, false, true);
    }
}

TimeMilli Socket::SendWindow::GetNextSendTime(TimeMilli aNow, uint32_t aRTT)
{
    TimeMilli nextSendTime = aNow.GetDistantFuture();
    Sequence  msgStartSeq  = mStartSeq;

    for (uint8_t i = 0; i < mLength; i++)
    {
        const Entry &entry           = GetEntry(i);
        TimeMilli    messageSendTime = entry.mLastSendTime + GetSendTimeout(entry, msgStartSeq, aRTT);

        VerifyOrExit(messageSendTime > aNow, nextSendTime = aNow);

        if (messageSendTime < nextSendTime)
        {
            nextSendTime = messageSendTime;
        }

        msgStartSeq += entry.GetSegmentLength();
    }

exit:
    otLogDebgTcp("Next send time: %dms", nextSendTime - aNow);
    return nextSendTime;
}

bool Socket::SendWindow::TakeCustody(Message &aMessage)
{
    bool found = false;

    for (uint8_t i = 0; i < mLength; i++)
    {
        Message *message = GetMessage(i);

        if (message == &aMessage)
        {
            OT_ASSERT(!message->IsInAQueue());

            otLogDebgTcp("TakeCustody before: Reserved=%d, Offset=%d, sizeof(ot::Ip6::Header)=%d",
                         message->GetReserved(), message->GetOffset(), sizeof(ot::Ip6::Header));

            SuccessOrExit(message->ResetMetadata(Message::kTypeIp6, Message::kPriorityNormal));
            message->SetIsManagedByTcp(true);

            CHECK_TCP_INVARIANT(message->GetReserved() <= Ip6::kMessageReserveHeaderLength);
            message->RemoveHeader(Ip6::kMessageReserveHeaderLength - message->GetReserved());
            message->SetOffset(sizeof(Header));

            otLogDebgTcp("TakeCustody after: Reserved=%d, Offset=%d, sizeof(ot::Ip6::Header)=%d",
                         message->GetReserved(), message->GetOffset(), sizeof(ot::Ip6::Header));

            ExitNow(found = true);
        }
    }

exit:
    return found;
}

bool Socket::SendWindow::IsEmpty() const
{
    return mLength == 0;
}

void Socket::SendWindow::AddSYN(void)
{
    CHECK_TCP_INVARIANT(IsEmpty());

    Add(nullptr, true, false);
}

void Socket::SendWindow::AddFIN(void)
{
    Entry *entry = GetLast();

    if (entry != nullptr && (entry->mIsSyn || entry->mSendCount > 0))
    {
        entry = nullptr;
    }

    if (entry == nullptr)
    {
        if (!IsFull())
        {
            Add(nullptr, false, true);
        }
        else
        {
            mPendingFIN = true;
        }
    }
    else
    {
        entry->mIsFin = true;
    }
}

void Socket::SendWindow::Add(Message &aMessage)
{
    Add(&aMessage, false, false);
}

void Socket::SendWindow::Add(Message *aMessage, bool aIsSyn, bool aIsFin)
{
    CHECK_TCP_INVARIANT(!IsFull());
    CHECK_TCP_INVARIANT(!aIsSyn || !aIsFin);
    CHECK_TCP_INVARIANT(!aIsSyn || aMessage == nullptr);

    CheckInvariant();

    {
        Entry &entry = GetEntry(mLength);

        entry.mMessage      = aMessage;
        entry.mSendCount    = 0;
        entry.mLastSendTime = TimerMilli::GetNow();
        entry.mIsSyn        = aIsSyn;
        entry.mIsFin        = aIsFin;

        mLength++;
    }

    CheckInvariant();
}

Socket::SendWindow::Entry *Socket::SendWindow::GetLast(void)
{
    return mLength > 0 ? &GetEntry(mLength - 1) : nullptr;
}

void Socket::SendWindow::Flush(void)
{
    for (uint8_t i = 0; i < mLength; i++)
    {
        FreeEntry(GetEntry(i));
    }

    Clear();
    CheckInvariant();
}

void Socket::SendWindow::FreeEntry(Entry &aEntry)
{
    Message *message = aEntry.mMessage;

    if (message != nullptr && !message->IsInAQueue())
    {
        message->SetIsManagedByTcp(false);
        message->Free();
    }

    aEntry.mMessage = nullptr;
    aEntry.mIsSyn = aEntry.mIsFin = false;
}

void Socket::SendWindow::ConfigSendWindowBySYN(const Header &aHeader)
{
    // set SND.WL1 to SEG.SEQ-1 so that the Send Window will be updated by SYN
    mSndWl1 = aHeader.GetSequenceNumber() - 1;
}

void Socket::SendWindow::UpdateSendWindow(const Header &aHeader)
{
    //   If SND.UNA < SEG.ACK =< SND.NXT, the send window should be
    //   updated.  If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and
    //   SND.WL2 =< SEG.ACK)), set SND.WND <- SEG.WND, set
    //   SND.WL1 <- SEG.SEQ, and set SND.WL2 <- SEG.ACK.

    Sequence ack = aHeader.GetAcknowledgmentNumber();
    Sequence seq = aHeader.GetSequenceNumber();

    VerifyOrExit(ack > GetStartSeq());

    if ((mSndWl1 < seq) || (mSndWl1 == seq && mSndWl2 <= ack))
    {
        mSndWnd = aHeader.GetWindow();
        mSndWl1 = seq;
        mSndWl2 = ack;
    }

exit:
    otLogDebgTcp("UpdateSendWindow: SEG.WIN=%d, SEG.SEQ=%u, SEG.ACK=%u, SND.UNA=%u, SND.WL1=%u, SND.WL2=%u, SND.WND=%d",
                 aHeader.GetWindow(), seq.GetValue(), ack.GetValue(), GetStartSeq().GetValue(), mSndWl1, mSndWl2,
                 mSndWnd);
}

uint16_t Socket::SendWindow::GetSendWindowSize(void) const
{
    return mSndWnd;
}

uint32_t Socket::SendWindow::GetSendTimeout(const Entry &aEntry, Sequence aMsgStartSeq, uint32_t aRTT)
{
    Sequence msgStopSeq = aMsgStartSeq + aEntry.GetSegmentLength();
    uint32_t timeout;

    if (msgStopSeq > mStartSeq + mSndWnd)
    {
        otLogDebgTcp("Msg %u-%u deleyd because SND.WND (%u+%d) is full!", aMsgStartSeq.GetValue(),
                     msgStopSeq.GetValue(), mStartSeq.GetValue(), mSndWnd);
        timeout = kZeroWindowSendInterval;
    }
    else if (aEntry.mSendCount == 0)
    {
        timeout = aEntry.IsWritable() ? static_cast<uint32_t>(kNewMessageSendTimeout) : 0;
    }
    else
    {
        timeout = aEntry.mIsSyn ? static_cast<uint32_t>(kSynTimeout) : aRTT;
    }

    return timeout;
}

void Socket::ReceiveWindow::Init(Sequence aStartSeq)
{
    CHECK_TCP_INVARIANT(mLength == 0);

    mStartSeq = aStartSeq;
}

Sequence Socket::ReceiveWindow::GetStartSeq()
{
    return mStartSeq;
}

otError Socket::ReceiveWindow::Add(Message &aMessage)
{
    otError  error = OT_ERROR_NONE;
    uint8_t  insertPos;
    Sequence msgStartSeq, msgStopSeq;

    CheckInvariant();

    GetSegmentRange(aMessage, msgStartSeq, msgStopSeq);

    for (insertPos = mLength; insertPos > mProcessNext; insertPos--)
    {
        Message  prevMsg = GetMessage(insertPos - 1);
        Sequence prevStartSeq, prevStopSeq;

        GetSegmentRange(prevMsg, prevStartSeq, prevStopSeq);

        if (prevStartSeq <= msgStartSeq && msgStopSeq <= prevStopSeq)
        {
            // `aMessage` is a subrange of `prevMsg`
            IgnoreError(MergeFlags(prevMsg, aMessage));
            ExitNow(error = OT_ERROR_DROP);
        }
        else if (msgStartSeq <= prevStartSeq && prevStopSeq <= msgStopSeq)
        {
            // `prevMsg` is a subrange of `aMessage`
            IgnoreError(MergeFlags(aMessage, prevMsg));
            Pop(insertPos - 1).Free();
            continue;
        }
        else if (msgStartSeq > prevStartSeq)
        {
            // `aMessage` should be inserted after `prevMsg`
            break;
        }

        CHECK_TCP_INVARIANT(msgStartSeq < prevStartSeq);
    }

    if (IsFull())
    {
        // if all segments are processed, we have to wait for user to read these segments
        VerifyOrExit(!IsProcessEmpty(), error = OT_ERROR_NO_BUFS);
        VerifyOrExit(insertPos != mLength, {
            IgnoreError(MergeFlags(GetMessage(mLength - 1), aMessage));
            error = OT_ERROR_NO_BUFS;
        });

        {
            Message &lastMsg = Pop(mLength - 1);
            IgnoreError(MergeFlags(insertPos == mLength ? aMessage : GetMessage(mLength - 1), lastMsg));
            lastMsg.Free();
        }
    }

    for (uint8_t i = mLength; i > insertPos; i--)
    {
        GetEntry(i) = GetEntry(i - 1);
    }

    GetEntry(insertPos).mMessage = &aMessage;
    mLength++;

exit:
    CheckInvariant();

    otLogDebgTcp("Add Segment %u-%u: %s, mProcessNext=%d, mLength=%d", msgStartSeq.GetValue(), msgStopSeq.GetValue(),
                 otThreadErrorToString(error), mProcessNext, mLength);

    return error;
}

void Socket::ReceiveWindow::CheckInvariant(void)
{
#if OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
    CHECK_TCP_INVARIANT(mStartIndex < OT_ARRAY_LENGTH(mSegments));
    CHECK_TCP_INVARIANT(mLength <= OT_ARRAY_LENGTH(mSegments));
    CHECK_TCP_INVARIANT(mProcessNext <= mLength);

    Sequence lastMsgStartSeq(0);

    for (uint8_t i = 0; i < mProcessNext; i++)
    {
        Entry &  entry = GetEntry(i);
        Sequence msgStartSeq, msgStopSeq;

        CHECK_TCP_INVARIANT(entry.mMessage != nullptr);
        GetSegmentRange(*entry.mMessage, msgStartSeq, msgStopSeq);

        CHECK_TCP_INVARIANT(msgStopSeq <= mStartSeq);
        CHECK_TCP_INVARIANT(!(i > 0) || msgStartSeq >= lastMsgStartSeq);

        lastMsgStartSeq = msgStartSeq;
    }

    for (uint8_t i = mProcessNext; i < mLength; i++)
    {
        Entry &  entry = GetEntry(i);
        Sequence msgStartSeq, msgStopSeq;

        CHECK_TCP_INVARIANT(entry.mMessage != nullptr);
        GetSegmentRange(*entry.mMessage, msgStartSeq, msgStopSeq);

        CHECK_TCP_INVARIANT(msgStopSeq >= mStartSeq);
        CHECK_TCP_INVARIANT(!(i > mProcessNext) || msgStartSeq >= lastMsgStartSeq);

        lastMsgStartSeq = msgStartSeq;
    }

    for (uint8_t i = mLength; i < OT_ARRAY_LENGTH(mSegments); i++)
    {
        Entry &entry = GetEntry(i);

        CHECK_TCP_INVARIANT(entry.mMessage == nullptr);
    }
#endif // OPENTHREAD_EXAMPLES_SIMULATION && OPENTHREAD_CONFIG_ASSERT_ENABLE
}

bool Socket::ReceiveWindow::IsFull(void) const
{
    return mLength == kMaxRecvSegments;
}

Message *Socket::ReceiveWindow::Process(void)
{
    Message *recvMsg = nullptr;
    Sequence msgStartSeq, msgStopSeq;

    CheckInvariant();

    VerifyOrExit(!IsProcessEmpty());

    {
        Message &message = GetMessage(mProcessNext);

        CHECK_TCP_INVARIANT(message.GetOffset() == sizeof(Header));

        GetSegmentRange(message, msgStartSeq, msgStopSeq);

        VerifyOrExit(msgStartSeq <= mStartSeq);

        CHECK_TCP_INVARIANT(msgStopSeq >= mStartSeq);

        otLogDebgTcp("Recv: msg %u-%u, RCV.NXT=%u", msgStartSeq.GetValue(), msgStopSeq.GetValue(),
                     mStartSeq.GetValue());

        mProcessNext++;

        message.MoveOffset(OT_MIN(mStartSeq - msgStartSeq, message.GetLength() - message.GetOffset()));
        mStartSeq = msgStopSeq;
        otLogDebgTcp("RCV.NXT = %u", mStartSeq.GetValue());
        recvMsg = &message;
    }

exit:
    CheckInvariant();
    return recvMsg;
}

Message &Socket::ReceiveWindow::Pop(uint8_t aIndex)
{
    Message *msg;

    CheckInvariant();

    OT_ASSERT(aIndex < mLength);

    msg = GetEntry(aIndex).mMessage;

    if (aIndex == 0)
    {
        GetEntry(aIndex).mMessage = nullptr;
        mStartIndex               = (mStartIndex + 1) % OT_ARRAY_LENGTH(mSegments);
    }
    else
    {
        // recv queue must be kept in order
        for (uint8_t i = aIndex; i < mLength - 1; i++)
        {
            GetEntry(i) = GetEntry(i + 1);
        }

        GetEntry(mLength - 1).mMessage = nullptr;
    }

    if (aIndex < mProcessNext)
    {
        mProcessNext--;
    }

    mLength--;

    CheckInvariant();

    return *msg;
}

void Socket::ReceiveWindow::GetSegmentRange(Message &aMessage, Sequence &aStartSeq, Sequence &aStopSeq)
{
    Header   tcpHeader;
    otError  error       = aMessage.Read(0, tcpHeader);
    uint16_t payloadSize = aMessage.GetLength() - aMessage.GetOffset();

    OT_ASSERT(error == OT_ERROR_NONE);
    OT_UNUSED_VARIABLE(error);

    aStartSeq = tcpHeader.GetSequenceNumber();
    aStopSeq  = aStartSeq + payloadSize + tcpHeader.HasFlags(kFlagSYN) + tcpHeader.HasFlags(kFlagFIN);
}

void Socket::ReceiveWindow::Flush(void)
{
    for (uint8_t i = 0; i < mLength; i++)
    {
        GetMessage(i).Free();
    }

    Clear();
    CheckInvariant();
}

void Socket::ReceiveWindow::ClearEmptySegments(void)
{
    // clear empty segments
    for (uint8_t i = 0; i < mProcessNext; i++)
    {
        Message &message = GetMessage(i);

        if (GetSegmentTextLength(message) == 0)
        {
            Pop(i).Free();
            i--;
        }
    }
}

uint32_t Socket::ReceiveWindow::GetReadable(void)
{
    uint32_t readable = 0;

    for (uint8_t i = 0; i < mProcessNext; i++)
    {
        Message &message = GetMessage(i);
        uint16_t segLen  = GetSegmentTextLength(message);

        CHECK_TCP_INVARIANT(segLen > 0);

        otLogDebgTcp("GetSegmentTextLength(%d/%d): %d", i, mProcessNext, segLen);

        readable += segLen;
    }

    return readable;
}

uint16_t Socket::ReceiveWindow::GetSegmentTextLength(Message &aMessage)
{
    return aMessage.GetLength() - aMessage.GetOffset();
}

uint16_t Socket::ReceiveWindow::Read(uint8_t *aBuf, uint16_t aSize)
{
    uint16_t ret = 0;

    otLogDebgTcp("ReceiveWindow::Read: buf=%d, mProcessNext=%d/d%", aSize, mProcessNext, mLength);

    while (mProcessNext > 0 && aSize > 0)
    {
        Message &message = GetMessage(0);
        uint16_t segLen  = GetSegmentTextLength(message);
        uint16_t readLen = OT_MIN(segLen, aSize);

        IgnoreError(message.Read(message.GetOffset(), aBuf, readLen));

        if (readLen == segLen)
        {
            Pop(0).Free();
        }
        else
        {
            message.MoveOffset(readLen);
        }

        aBuf += readLen;
        aSize -= readLen;
        ret += readLen;
    }

    return ret;
}

uint16_t Socket::ReceiveWindow::GetReceiveWindowSize(void) const
{
    //    The number of data octets beginning with the one indicated in the
    //    acknowledgment field which the sender of this segment is willing to
    //    accept.

    uint8_t maxSegments = kMaxRecvSegments - mProcessNext;
    return maxSegments * kMaxSegmentSize;
}

otError Socket::ReceiveWindow::MergeFlags(Message &aMessage, const Message &aMerging)
{
    otError error = OT_ERROR_NONE;
    uint8_t mergedFlags, mergingFlags;
    bool    flagsUpdated = false;

    SuccessOrExit(error = aMessage.Read(0 + OT_FIELD_OFFSET(Header, mFlags), mergedFlags));
    SuccessOrExit(error = aMerging.Read(0 + OT_FIELD_OFFSET(Header, mFlags), mergingFlags));

    if ((mergingFlags & kFlagRST) != 0)
    {
        mergedFlags |= kFlagRST;
        flagsUpdated = true;
    }

    if ((mergingFlags & kFlagACK) != 0)
    {
        Sequence mergedAckNumber, mergingAckNumber;
        bool     ackUpdated = false;

        SuccessOrExit(error = aMerging.Read(0 + OT_FIELD_OFFSET(Header, mAckNumber), mergingAckNumber));

        if ((mergedFlags & kFlagACK) != 0)
        {
            SuccessOrExit(error = aMessage.Read(0 + OT_FIELD_OFFSET(Header, mAckNumber), mergedAckNumber));

            if (mergingAckNumber > mergedAckNumber)
            {
                otLogDebgTcp("MergeFlags: ACK:%u -> %u", mergedAckNumber.GetValue(), mergingAckNumber.GetValue());
                mergedAckNumber = mergingAckNumber;
                ackUpdated      = true;
            }
            else
            {
                otLogDebgTcp("MergeFlags: ACK: %u NOT %u", mergedAckNumber.GetValue(), mergingAckNumber.GetValue());
            }
        }
        else
        {
            mergedFlags |= kFlagACK;
            otLogDebgTcp("MergeFlags: ACK:%u -> %u", 0, mergingAckNumber.GetValue());
            mergedAckNumber = mergingAckNumber;
            flagsUpdated = ackUpdated = true;
        }

        if (ackUpdated)
        {
            aMessage.Write(0 + OT_FIELD_OFFSET(Header, mAckNumber), mergedAckNumber);
        }
    }

    if (flagsUpdated)
    {
        aMessage.Write(0 + OT_FIELD_OFFSET(Header, mFlags), mergedFlags);
    }

exit:
    CHECK_TCP_INVARIANT(error == OT_ERROR_NONE);

    return error;
}

uint16_t Socket::SendWindow::Entry::GetSegmentLength(void) const
{
    uint16_t segSize = mIsSyn + mIsFin;

    VerifyOrExit(mMessage != nullptr);

    segSize += mMessage->GetLength() + mMessage->GetReserved() - Ip6::kMessageReserveHeaderLength - sizeof(Header);

exit:
    return segSize;
}

bool Socket::SendWindow::Entry::IsWritable(void) const
{
    return (!mIsSyn && !mIsFin && mSendCount == 0 && GetSegmentLength() < kMaxSegmentSize);
}

#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
void Tcp::SetSegmentRandomDropProb(uint8_t aProb)
{
    mSegmentRandomDropProb = aProb;
}

void Socket::ResetNextSegment(void)
{
    mResetNextSegment = true;
}
#endif

} // namespace Ip6
} // namespace ot

#endif // (OPENTHREAD_FTD || OPENTHREAD_MTD) && OPENTHREAD_CONFIG_TCP_ENABLE
