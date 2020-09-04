/*
 *
 *    Copyright (c) 2016-2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 */

#include "ot-legacy-pairing-ext.h"

#include "openthread-core-config.h"

#include <openthread/crypto.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/message.h>
#include <openthread/ncp.h>
#include <openthread/udp.h>

#include "common/code_utils.hpp"
#include "common/extension.hpp"
#include "common/locator-getters.hpp"
#include "common/logging.hpp"
#include "common/new.hpp"
#include "common/notifier.hpp"
#include "common/random.hpp"
#include "common/timer.hpp"
#include "net/ip6_address.hpp"
#include "thread/key_manager.hpp"

namespace ot {
namespace Extension {

#if !OPENTHREAD_ENABLE_VENDOR_EXTENSION
#error Vendor extension source file is being built without enabling extension in OpenThread.
#endif

//------------------------------------------------------------------------

/**
 * This class defines the vendor extension to add support for legacy pairing.
 *
 */
class LegacyPairingExtension : public ExtensionBase
{
    friend class ExtensionBase;

public:
    explicit LegacyPairingExtension(Instance &aInstance);

    static LegacyPairingExtension &GetExtension(void) { return *sLegacyPairingExtension; }

    void AfterInstanceInit(void);
    void AfterNcpInit(Ncp::NcpBase &aNcpBase);

    // Legacy handlers
    static void InitLegacy(void) { IgnoreError(sLegacyPairingExtension->Init()); }
    static void StartLegacy(void) { sLegacyPairingExtension->Start(); }
    static void StopLegacy(void) { sLegacyPairingExtension->Stop(); }
    static void JoinLegacyNode(const otExtAddress *aDstAddress) { sLegacyPairingExtension->Join(aDstAddress); }
    static void SetLegacyUlaPrefix(const uint8_t *aUlaPrefix) { sLegacyPairingExtension->SetPrefix(aUlaPrefix); }

private:
    enum
    {
        MLE_UDP_PORT             = 19788,
        MLE_MSG_BUFFER_SIZE      = 80,
        MLE_KEY_LENGTH           = 16,
        IPV6_INTERFACE_ID_OFFSET = 8,
        SECURITY_MIC_LENGTH      = 4,
        SECURITY_NONCE_LENGTH    = 13,
        MAX_HOP_LIMIT            = 255,
        MLE_ADVR_TX_PERIOD_IN_MS = (40 * 1000), // 40 seconds
        MLE_JOIN_TX_PERIOD_IN_MS = 600,         // 600 ms
        MLE_MAX_JOIN_ATTEMPTS    = 5,

        HEADER_SEC_SUITE_ENABLE        = 0,
        HEADER_SEC_CTRL_ENC_MIC_32     = (5 << 0),
        HEADER_SEC_CTRL_KEY_ID_MODE_1  = (1 << 3),
        HEADER_KEY_INDEX_DEFAULT_VALUE = 1,

        // Security header: security ctrl (u8) | frame counter (u32) | key id (u8)
        SECURITY_HEADER_LENGTH = (1 + 4 + 1),

        // AES-CCM header includes: src ip6 address, dst ip6 address, security header
        AES_CCM_HEADER_LENGTH = (sizeof(otIp6Address) * 2 + SECURITY_HEADER_LENGTH),

        // MLE commands
        MLE_CMD_LINK_REQUEST        = 0,
        MLE_CMD_LINK_ACCEPT_REQUEST = 2,
        MLE_CMD_LINK_ACCEPT         = 1,
        MLE_CMD_ADVERTISEMENT       = 4,

        // TLVs
        TLV_MODE_TYPE   = 1,
        TLV_MODE_LENGTH = 1,
        TLV_MODE_VALUE  = 0x00,

        TLV_TIMEOUT_TYPE          = 2,
        TLV_TIMEOUT_LENGTH        = 4,
        TLV_TIMEOUT_VALUE_DEFAULT = 240,

        TLV_CHALLENGE_TYPE   = 3,
        TLV_CHALLENGE_LENGTH = 8,

        TLV_RESPONSE_TYPE   = 4,
        TLV_RESPONSE_LENGTH = 8,

        TLV_LL_FRAME_COUNTER_TYPE   = 5,
        TLV_LL_FRAME_COUNTER_LENGTH = 4,

        TLV_ULA_PREFIX_TYPE   = 131,
        TLV_ULA_PREFIX_LENGTH = 8,
    };

    enum LegacyState
    {
        kStateDisabled,
        kStateJoining,
        kStateEnabled,
    };

    otError  Init(void);
    void     Start(void);
    void     Stop(void);
    void     Join(const otExtAddress *aDstAddress);
    void     SetPrefix(const uint8_t *aUlaPrefix);
    void     LogMleMessage(const char *aLogString, const otIp6Address *aIp6Address) const;
    void     ComputeLegacyKey(void);
    otError  AddReceiver(void);
    bool     HandleUdpReceive(const otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void     HandleMleLinkRequest(const uint8_t *aBufPtr, uint16_t aLength, const otMessageInfo *aMessageInfo);
    void     HandleMleLinkAcceptAndRequest(uint8_t              aCommand,
                                           const uint8_t *      aBufPtr,
                                           uint16_t             aLength,
                                           const otMessageInfo *aMessageInfo);
    void     SendMleCommand(uint8_t aCommand, const uint8_t *aResponse, const otIp6Address *aDstIpAddr);
    uint8_t *AppendTlvs(uint8_t *aBufPtr, uint8_t aCommand, const uint8_t *aResponse);
    void     SendAdvertisment(void);
    void     HandleTimer(void);

    const uint8_t *GetLegacyMleKey(void) const { return mLegacyKey; }

    uint32_t GetFrameCounter(void) { return mMleFrameCounter; }
    uint32_t IncrementFrameCounter(void);

    // Helper static methods
    static void GetExtAddressFromIpAddress(otExtAddress *aExtAddress, const otIp6Address *aIp6Address);
    static void GenerateLinkLocalIpAddress(otIp6Address *aIp6Address, const otExtAddress *aExtAddress);
    static void SetIp6AddressToLinkLocalMulticastAllNodes(otIp6Address *aIp6Address);
    static void SetIp6AddressToLinkLocalMulticastAllRouters(otIp6Address *aIp6Address);
    static bool IsUlaPrefixValid(const uint8_t *aUlaPrefix);
    static void GenerateNonce(const otExtAddress *aMacAddr,
                              uint32_t            aFrameCounter,
                              uint8_t             aSecurityLevel,
                              uint8_t *           aNonce);

    // Callbacks
    static bool HandleUdpReceive(void *aContext, const otMessage *aMessage, const otMessageInfo *aMessageInfo);
    static void HandleTimer(Timer &aTimer);

    uint8_t            mLegacyKey[OT_CRYPTO_HMAC_SHA_HASH_SIZE];
    LegacyState        mState;
    Ip6::Udp::Receiver mLegacyMleReceiver;
    uint32_t           mMleFrameCounter;
    uint8_t            mChallenge[TLV_CHALLENGE_LENGTH];
    uint8_t            mLegacyUlaPrefix[TLV_ULA_PREFIX_LENGTH];
    uint8_t            mJoinAttemptsCounter;
    TimerMilli         mTimer;

    static LegacyPairingExtension *  sLegacyPairingExtension;
    static const otNcpLegacyHandlers sLegacyHandlers;
};

// ----------------------------------------------------------------------------
// `ExtensionBase` API
// ----------------------------------------------------------------------------

static OT_DEFINE_ALIGNED_VAR(sExtensionRaw, sizeof(LegacyPairingExtension), uint64_t);

ExtensionBase &ExtensionBase::Init(Instance &aInstance)
{
    ExtensionBase *ext = reinterpret_cast<ExtensionBase *>(&sExtensionRaw);

    VerifyOrExit(!ext->mIsInitialized, OT_NOOP);

    ext = new (&sExtensionRaw) LegacyPairingExtension(aInstance);

exit:
    return *ext;
}

void ExtensionBase::SignalInstanceInit(void)
{
    static_cast<LegacyPairingExtension *>(this)->AfterInstanceInit();
}

void ExtensionBase::SignalNcpInit(Ncp::NcpBase &aNcpBase)
{
    static_cast<LegacyPairingExtension *>(this)->AfterNcpInit(aNcpBase);
}

void ExtensionBase::HandleNotifierEvents(Events aEvents)
{
    otLogInfoMle("[Legacy] Notifier Events: %08x", aEvents.GetAsFlags());

    if (aEvents.Contains(kEventMasterKeyChanged))
    {
        static_cast<LegacyPairingExtension *>(this)->ComputeLegacyKey();
    }
}

// ----------------------------------------------------------------------------
// LegacyPairingExtension class
// ----------------------------------------------------------------------------

LegacyPairingExtension *LegacyPairingExtension::sLegacyPairingExtension = NULL;

const otNcpLegacyHandlers LegacyPairingExtension::sLegacyHandlers = {
    &LegacyPairingExtension::StartLegacy,        // otNcpHandlerStartLegacy         mStartLegacy;
    &LegacyPairingExtension::StopLegacy,         // otNcpHandlerStopLegacy          mStopLegacy;
    &LegacyPairingExtension::JoinLegacyNode,     // otNcpHandlerJoinLegacyNode      mJoinLegacyNode;
    &LegacyPairingExtension::SetLegacyUlaPrefix, // otNcpHandlerSetLegacyUlaPrefix  mSetLegacyUlaPrefix;
};

LegacyPairingExtension::LegacyPairingExtension(Instance &aInstance)
    : ExtensionBase(aInstance)
    , mState(kStateDisabled)
    , mLegacyMleReceiver(&LegacyPairingExtension::HandleUdpReceive, this)
    , mTimer(aInstance, LegacyPairingExtension::HandleTimer, this)
{
    memset(mLegacyKey, 0, sizeof(mLegacyKey));

    OT_ASSERT(sLegacyPairingExtension == NULL);
    sLegacyPairingExtension = this;
}

void LegacyPairingExtension::AfterInstanceInit(void)
{
    ComputeLegacyKey();
}

void LegacyPairingExtension::AfterNcpInit(Ncp::NcpBase &aNcpBase)
{
    OT_UNUSED_VARIABLE(aNcpBase);
    IgnoreError(Init());
}

void LegacyPairingExtension::LogMleMessage(const char *aLogString, const otIp6Address *aIp6Address) const
{
    const Ip6::Address *address = reinterpret_cast<const Ip6::Address *>(aIp6Address);

    OT_UNUSED_VARIABLE(aLogString);
    OT_UNUSED_VARIABLE(aIp6Address);
    OT_UNUSED_VARIABLE(address);

    otLogInfoMle("%s (%s)", aLogString, address->ToString().AsCString());
}

void LegacyPairingExtension::ComputeLegacyKey(void)
{
    static const uint8_t kLegacyString[] = {
        'Z', 'i', 'g', 'B', 'e', 'e', 'I', 'P',
    };

    otCryptoHmacSha256(Get<KeyManager>().GetMasterKey().m8, OT_MASTER_KEY_SIZE, kLegacyString, sizeof(kLegacyString),
                       mLegacyKey);

    otLogInfoMle("[Legacy] Update Legacy Key");
}

void LegacyPairingExtension::GenerateNonce(const otExtAddress *aMacAddr,
                                           uint32_t            aFrameCounter,
                                           uint8_t             aSecurityLevel,
                                           uint8_t *           aNonce)
{
    // source address
    memcpy(aNonce, aMacAddr->m8, sizeof(otExtAddress));
    aNonce += sizeof(otExtAddress);

    // frame counter
    aNonce[0] = (aFrameCounter >> 24) & 0xff;
    aNonce[1] = (aFrameCounter >> 16) & 0xff;
    aNonce[2] = (aFrameCounter >> 8) & 0xff;
    aNonce[3] = aFrameCounter & 0xff;
    aNonce += sizeof(uint32_t);

    // security level
    aNonce[0] = aSecurityLevel;
}

otError LegacyPairingExtension::Init(void)
{
    otError error = OT_ERROR_NONE;

    mState               = kStateDisabled;
    mJoinAttemptsCounter = 0;

    SuccessOrExit(error = AddReceiver());
    SuccessOrExit(error = Random::Crypto::FillBuffer(mChallenge, TLV_CHALLENGE_LENGTH));

    mMleFrameCounter = 0;

    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));

    // Register the handlers with ot ncp
    otNcpRegisterLegacyHandlers(&sLegacyHandlers);

exit:
    return error;
}

void LegacyPairingExtension::Start(void)
{
    SendAdvertisment();
    Join(NULL);
    mState               = kStateJoining;
    mJoinAttemptsCounter = 0;
    mTimer.Start(MLE_JOIN_TX_PERIOD_IN_MS);

    otLogInfoMle("[Legacy] Start");
}

void LegacyPairingExtension::Stop(void)
{
    mTimer.Stop();
    memset(mLegacyUlaPrefix, 0, sizeof(mLegacyUlaPrefix));
    mMleFrameCounter = 0;
    mState           = kStateDisabled;

    otLogInfoMle("[Legacy] Stop");
}

void LegacyPairingExtension::Join(const otExtAddress *aDstAddress)
{
    otIp6Address dstIpAddress;

    if (aDstAddress != NULL)
    {
        GenerateLinkLocalIpAddress(&dstIpAddress, aDstAddress);
    }
    else
    {
        SetIp6AddressToLinkLocalMulticastAllRouters(&dstIpAddress);
    }

    LogMleMessage("[Legacy] Send Link Request", &dstIpAddress);

    SendMleCommand(MLE_CMD_LINK_REQUEST, NULL, &dstIpAddress);
}

void LegacyPairingExtension::SetPrefix(const uint8_t *aUlaPrefix)
{
    memcpy(mLegacyUlaPrefix, aUlaPrefix, sizeof(mLegacyUlaPrefix));
}

uint32_t LegacyPairingExtension::IncrementFrameCounter(void)
{
    uint32_t prevValue = mMleFrameCounter;
    mMleFrameCounter++;
    return prevValue;
}

otError LegacyPairingExtension::AddReceiver(void)
{
    otError error = OT_ERROR_NONE;

    error = Get<Ip6::Udp>().AddReceiver(mLegacyMleReceiver);
    SuccessOrExit(error);

exit:
    return error;
}

void LegacyPairingExtension::GetExtAddressFromIpAddress(otExtAddress *aExtAddress, const otIp6Address *aIp6Address)
{
    memcpy(aExtAddress->m8, aIp6Address->mFields.m8 + IPV6_INTERFACE_ID_OFFSET, sizeof(otExtAddress));
    aExtAddress->m8[0] ^= 0x02;
}

void LegacyPairingExtension::GenerateLinkLocalIpAddress(otIp6Address *aIp6Address, const otExtAddress *aExtAddress)
{
    memset(aIp6Address, 0, sizeof(otIp6Address));
    aIp6Address->mFields.m8[0] = 0xfe;
    aIp6Address->mFields.m8[1] = 0x80;
    memcpy(aIp6Address->mFields.m8 + IPV6_INTERFACE_ID_OFFSET, aExtAddress->m8, sizeof(otExtAddress));
    aIp6Address->mFields.m8[IPV6_INTERFACE_ID_OFFSET] ^= 0x02;
}

void LegacyPairingExtension::SetIp6AddressToLinkLocalMulticastAllNodes(otIp6Address *aIp6Address)
{
    memset(aIp6Address, 0, sizeof(otIp6Address));
    aIp6Address->mFields.m8[0]  = 0xff;
    aIp6Address->mFields.m8[1]  = 0x02;
    aIp6Address->mFields.m8[15] = 0x01;
}

void LegacyPairingExtension::SetIp6AddressToLinkLocalMulticastAllRouters(otIp6Address *aIp6Address)
{
    memset(aIp6Address, 0, sizeof(otIp6Address));
    aIp6Address->mFields.m8[0]  = 0xff;
    aIp6Address->mFields.m8[1]  = 0x02;
    aIp6Address->mFields.m8[15] = 0x02;
}

bool LegacyPairingExtension::IsUlaPrefixValid(const uint8_t *aUlaPrefix)
{
    // Check all the positions, if any non-zero, it's valid.
    for (int i = 0; i < TLV_ULA_PREFIX_LENGTH; i++)
    {
        if (aUlaPrefix[i] != 0)
        {
            return true;
        }
    }

    // if all are zero, then it's an invalid (empty) prefix.
    return false;
}

bool LegacyPairingExtension::HandleUdpReceive(void *               aContext,
                                              const otMessage *    aMessage,
                                              const otMessageInfo *aMessageInfo)
{
    bool rval = false;

    if (aMessageInfo->mSockPort == MLE_UDP_PORT)
    {
        rval = reinterpret_cast<LegacyPairingExtension *>(aContext)->HandleUdpReceive(aMessage, aMessageInfo);
    }

    return rval;
}

bool LegacyPairingExtension::HandleUdpReceive(const otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    uint16_t     length;
    uint8_t      buffer[MLE_MSG_BUFFER_SIZE + 2 * sizeof(otIp6Address)];
    uint8_t *    bufPtr;
    uint8_t *    micPtr;
    uint8_t      calculatedMic[SECURITY_MIC_LENGTH];
    uint8_t      securitySuite;
    uint32_t     frameCounter;
    uint8_t      command;
    uint8_t      nonce[SECURITY_NONCE_LENGTH];
    otExtAddress macAddr;
    bool         rval = false;

    //-----------------------------
    // Check the message length and copy the content

    // Ensure message length is smaller than max legacy MLE packet
    length = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
    VerifyOrExit(length < MLE_MSG_BUFFER_SIZE, OT_NOOP);

    // Read and check the security suite
    otMessageRead(aMessage, otMessageGetOffset(aMessage), &securitySuite, sizeof(securitySuite));
    VerifyOrExit(securitySuite == HEADER_SEC_SUITE_ENABLE, OT_NOOP);

    // Leave space for two IP address at start of buffer (used for AES security header)
    bufPtr = &buffer[2 * sizeof(otIp6Address)];

    // Copy the rest of message content into local buffer (skip the security suit which is already checked
    // and should be excluded from the AES-CCM header)
    length = otMessageRead(aMessage, otMessageGetOffset(aMessage) + 1, bufPtr, MLE_MSG_BUFFER_SIZE);
    VerifyOrExit(length < MLE_MSG_BUFFER_SIZE, OT_NOOP);

    //-----------------------------
    // Parse the security header

    // Make sure the packet includes security header, 1 byte for MLE command and MIC
    VerifyOrExit(length >= SECURITY_HEADER_LENGTH + SECURITY_MIC_LENGTH + 1, OT_NOOP);

    // Check the security control: KeyIdMode 2 | EncMic32
    VerifyOrExit(bufPtr[0] == (HEADER_SEC_CTRL_KEY_ID_MODE_1 | HEADER_SEC_CTRL_ENC_MIC_32), OT_NOOP);
    bufPtr += sizeof(uint8_t);

    // Read the frame counter (little endian)
    frameCounter = static_cast<uint32_t>((bufPtr[0] << 0) + (bufPtr[1] << 8) + (bufPtr[2] << 16) + (bufPtr[3] << 24));
    bufPtr += sizeof(uint32_t);

    // Check Key Index
    VerifyOrExit(bufPtr[0] == HEADER_KEY_INDEX_DEFAULT_VALUE, OT_NOOP);
    bufPtr++;

    //----------------------------
    // Generate Nonce

    GetExtAddressFromIpAddress(&macAddr, &aMessageInfo->mPeerAddr);
    GenerateNonce(&macAddr, frameCounter, HEADER_SEC_CTRL_ENC_MIC_32, nonce);

    //----------------------------
    // Read the MIC

    // MIC (security tag) is at the end of buffer (last SECURITY_MIC_LENGTH bytes)
    micPtr = &buffer[2 * sizeof(otIp6Address) + length - SECURITY_MIC_LENGTH];

    //-----------------------------
    // Prepare AES-CCM header:

    // AES-CCM header: peer Ip6 address followed by socket address
    bufPtr = &buffer[0];
    memcpy(bufPtr, &aMessageInfo->mPeerAddr, sizeof(otIp6Address));
    bufPtr += sizeof(otIp6Address);
    memcpy(bufPtr, &aMessageInfo->mSockAddr, sizeof(otIp6Address));

    //-----------------------------
    // Perform AES-CCM decryption

    otCryptoAesCcm(GetLegacyMleKey(), MLE_KEY_LENGTH,                     // Key and its length
                   SECURITY_MIC_LENGTH,                                   // MIC/Tag length
                   nonce, SECURITY_NONCE_LENGTH,                          // Nonce and its length
                   buffer, AES_CCM_HEADER_LENGTH,                         // AES-CCM header and its length
                   &buffer[AES_CCM_HEADER_LENGTH],                        // Start of plain text
                   &buffer[AES_CCM_HEADER_LENGTH],                        // Start of cipher text buffer
                   length - SECURITY_HEADER_LENGTH - SECURITY_MIC_LENGTH, // Plain text length
                   false,                                                 // Decrypt
                   calculatedMic                                          // Buffer for calculated MIC/tag.
    );

    // Verify the calculated MIC matches the received one
    VerifyOrExit(memcmp(calculatedMic, micPtr, SECURITY_MIC_LENGTH) == 0, OT_NOOP);

    //---------------------------------
    // Get the decrypted payload and process the MLE command

    length -= (SECURITY_HEADER_LENGTH + SECURITY_MIC_LENGTH);
    bufPtr = &buffer[AES_CCM_HEADER_LENGTH];

    command = *bufPtr++;
    length--;

    switch (command)
    {
    case MLE_CMD_LINK_REQUEST:
        HandleMleLinkRequest(bufPtr, length, aMessageInfo);
        break;

    case MLE_CMD_LINK_ACCEPT_REQUEST:
    case MLE_CMD_LINK_ACCEPT:
        HandleMleLinkAcceptAndRequest(command, bufPtr, length, aMessageInfo);
        break;

    case MLE_CMD_ADVERTISEMENT:
        // Handle advertisement (should be used to update timeout on legacy nodes for full legacy support).
        break;
    }

    rval = true;

exit:
    return rval;
}

void LegacyPairingExtension::HandleMleLinkRequest(const uint8_t *      aBufPtr,
                                                  uint16_t             aLength,
                                                  const otMessageInfo *aMessageInfo)
{
    const uint8_t *bufferEnd     = aBufPtr + aLength;
    const uint8_t *challenge     = NULL;
    bool           hasModeTlv    = false;
    bool           hasTimeoutTlv = false;
    uint32_t       timeout;
    uint8_t        type;
    uint8_t        len;

    otLogInfoMle("[Legacy] Receive Link Request");

    // Parse tlvs
    while (aBufPtr < bufferEnd)
    {
        type = *aBufPtr++;
        len  = *aBufPtr++;

        switch (type)
        {
        case TLV_MODE_TYPE:
            VerifyOrExit(len == TLV_MODE_LENGTH, OT_NOOP);
            hasModeTlv = true;
            break;

        case TLV_CHALLENGE_TYPE:
            VerifyOrExit(len == TLV_CHALLENGE_LENGTH, OT_NOOP);
            challenge = aBufPtr;
            break;

        case TLV_TIMEOUT_TYPE:
            VerifyOrExit(len == TLV_TIMEOUT_LENGTH, OT_NOOP);
            timeout =
                static_cast<uint32_t>((aBufPtr[0] << 24) + (aBufPtr[1] << 16) + (aBufPtr[2] << 8) + (aBufPtr[3] << 0));
            hasTimeoutTlv = true;
            break;
        }

        aBufPtr += len;
    }

    // Mode and Timeout tlvs are considered optional in Link Request.

    VerifyOrExit(challenge != NULL, OT_NOOP);

    LogMleMessage("[Legacy] Send Link Accept Request", &aMessageInfo->mPeerAddr);

    SendMleCommand(MLE_CMD_LINK_ACCEPT_REQUEST, challenge, &aMessageInfo->mPeerAddr);

    // For full legacy support, here we need to save the info (timeout, mac address) about this node in
    // legacy node list. The node will be considered fully joined, when we receive a Link Accept from it.
    OT_UNUSED_VARIABLE(timeout);
    OT_UNUSED_VARIABLE(hasTimeoutTlv);
    OT_UNUSED_VARIABLE(hasModeTlv);

exit:
    return;
}

void LegacyPairingExtension::HandleMleLinkAcceptAndRequest(uint8_t              aCommand,
                                                           const uint8_t *      aBufPtr,
                                                           uint16_t             aLength,
                                                           const otMessageInfo *aMessageInfo)
{
    const uint8_t *bufferEnd          = aBufPtr + aLength;
    const uint8_t *challenge          = NULL;
    const uint8_t *response           = NULL;
    const uint8_t *ulaPrefix          = NULL;
    bool           hasModeTlv         = false;
    bool           hasFrameCounterTlv = false;
    uint32_t       frameCounter;
    otExtAddress   dstAddr;
    uint8_t        type;
    uint8_t        len;

    VerifyOrExit((aCommand == MLE_CMD_LINK_ACCEPT_REQUEST) || (aCommand == MLE_CMD_LINK_ACCEPT), OT_NOOP);

    otLogInfoMle("[Legacy] Receive Link Accept and Request");

    // Parse tlvs
    while (aBufPtr < bufferEnd)
    {
        type = *aBufPtr++;
        len  = *aBufPtr++;

        switch (type)
        {
        case TLV_MODE_TYPE:
            VerifyOrExit(len == TLV_MODE_LENGTH, OT_NOOP);
            hasModeTlv = true;
            break;

        case TLV_CHALLENGE_TYPE:
            VerifyOrExit(len == TLV_CHALLENGE_LENGTH, OT_NOOP);
            challenge = aBufPtr;
            break;

        case TLV_RESPONSE_TYPE:
            VerifyOrExit(len == TLV_RESPONSE_LENGTH, OT_NOOP);
            response = aBufPtr;
            break;

        case TLV_LL_FRAME_COUNTER_TYPE:
            VerifyOrExit(len == TLV_LL_FRAME_COUNTER_LENGTH, OT_NOOP);
            hasFrameCounterTlv = true;
            frameCounter =
                static_cast<uint32_t>((aBufPtr[0] << 24) + (aBufPtr[1] << 16) + (aBufPtr[2] << 8) + (aBufPtr[3] << 0));
            break;

        case TLV_ULA_PREFIX_TYPE:
            VerifyOrExit(len == TLV_ULA_PREFIX_LENGTH, OT_NOOP);
            ulaPrefix = aBufPtr;
            break;
        }

        aBufPtr += len;
    }

    // Frame counter is required in both LinkAcceptAndRequest and in LinkAccept.
    VerifyOrExit(hasFrameCounterTlv, OT_NOOP);

    // Ensure the response matches the previously sent challenge.
    VerifyOrExit(response != NULL, OT_NOOP);
    VerifyOrExit(memcmp(response, mChallenge, TLV_CHALLENGE_LENGTH) == 0, OT_NOOP);

    // If prefix ULA TLV is available, update local copy (if different).
    if ((ulaPrefix != NULL) && IsUlaPrefixValid(ulaPrefix))
    {
        if (memcmp(mLegacyUlaPrefix, ulaPrefix, TLV_ULA_PREFIX_LENGTH) != 0)
        {
            memcpy(mLegacyUlaPrefix, ulaPrefix, TLV_ULA_PREFIX_LENGTH);

            otNcpHandleDidReceiveNewLegacyUlaPrefix(mLegacyUlaPrefix);
        }
    }

    switch (aCommand)
    {
    case MLE_CMD_LINK_ACCEPT_REQUEST:

        // LinkAcceptAndRequest must include a challenge TLV
        VerifyOrExit(challenge != NULL, OT_NOOP);

        LogMleMessage("[Legacy] Send Link Accept", &aMessageInfo->mPeerAddr);

        // Send a LinkAccept in response to the received LinkAcceptAndRequest.
        SendMleCommand(MLE_CMD_LINK_ACCEPT, challenge, &aMessageInfo->mPeerAddr);

        break;

    case MLE_CMD_LINK_ACCEPT:

        break;
    }

    // Get the mac address of the new legacy node and invoke the callback to notify that
    // the new node did join successfully.

    GetExtAddressFromIpAddress(&dstAddr, &aMessageInfo->mPeerAddr);
    otNcpHandleLegacyNodeDidJoin(&dstAddr);
    LogMleMessage("[Legacy] Legacy Node Did Join", &aMessageInfo->mPeerAddr);

    mState = kStateEnabled;

    OT_UNUSED_VARIABLE(frameCounter);
    OT_UNUSED_VARIABLE(hasModeTlv);

exit:
    return;
}

void LegacyPairingExtension::SendMleCommand(uint8_t aCommand, const uint8_t *aResponse, const otIp6Address *aDstIpAddr)
{
    uint8_t             buffer[MLE_MSG_BUFFER_SIZE + 2 * sizeof(otIp6Address)];
    uint8_t *           bufPtr;
    uint8_t             length;
    uint8_t             nonce[SECURITY_NONCE_LENGTH];
    const otExtAddress *srcAddr = NULL;
    otMessageInfo       msgInfo;
    otMessage *         udpMessage = NULL;
    uint8_t             securitySuite;
    otError             error;
    uint32_t            frameCounter = GetFrameCounter();
    otMessageSettings   msgSettings;

    //----------------------------
    // Prepare the msg info

    memset(&msgInfo, 0, sizeof(otMessageInfo));
    srcAddr = (const otExtAddress *)otLinkGetExtendedAddress(&GetInstance());
    GenerateLinkLocalIpAddress(&msgInfo.mSockAddr, srcAddr);
    memcpy(&msgInfo.mPeerAddr, aDstIpAddr, sizeof(otIp6Address));
    msgInfo.mPeerPort = MLE_UDP_PORT;
    msgInfo.mSockPort = MLE_UDP_PORT;
    msgInfo.mHopLimit = MAX_HOP_LIMIT;

    //----------------------------
    // Generate nonce
    GenerateNonce(srcAddr, frameCounter, HEADER_SEC_CTRL_ENC_MIC_32, nonce);

    //-----------------------------
    // Prepare AES-CCM header

    bufPtr = &buffer[0];

    // Add AES-CCM header: src ip6 address, dst ip6 address
    memcpy(bufPtr, &msgInfo.mSockAddr, sizeof(otIp6Address));
    bufPtr += sizeof(otIp6Address);
    memcpy(bufPtr, &msgInfo.mPeerAddr, sizeof(otIp6Address));
    bufPtr += sizeof(otIp6Address);

    //-----------------------------
    // Prepare the IEEE802.15.4 security header

    // Security control: KeyIdMode 2 | EncMic32
    *bufPtr++ = (HEADER_SEC_CTRL_KEY_ID_MODE_1 | HEADER_SEC_CTRL_ENC_MIC_32);

    // Frame counter (little endian)
    bufPtr[0] = (frameCounter >> 0) & 0xff;
    bufPtr[1] = (frameCounter >> 8) & 0xff;
    bufPtr[2] = (frameCounter >> 16) & 0xff;
    bufPtr[3] = (frameCounter >> 24) & 0xff;
    bufPtr += sizeof(uint32_t);

    // Key Index
    *bufPtr++ = HEADER_KEY_INDEX_DEFAULT_VALUE;

    //-----------------------------
    // Prepare the MLE payload (command and tlvs)

    *bufPtr++ = aCommand;
    bufPtr    = AppendTlvs(bufPtr, aCommand, aResponse);

    //-----------------------------
    // Perform AES-CCM decryption

    // Find the payload (plain-text length - excluding the security header)
    length = static_cast<uint8_t>(bufPtr - &buffer[0]);
    length -= AES_CCM_HEADER_LENGTH;

    // Encrypt and append the MIC
    otCryptoAesCcm(GetLegacyMleKey(), MLE_KEY_LENGTH, // Key and its length
                   SECURITY_MIC_LENGTH,               // MIC/Tag length
                   nonce, SECURITY_NONCE_LENGTH,      // Nonce and its length
                   buffer, AES_CCM_HEADER_LENGTH,     // AES-CCM header and its length
                   &buffer[AES_CCM_HEADER_LENGTH],    // Start of plain text
                   &buffer[AES_CCM_HEADER_LENGTH],    // Start of cipher text buffer
                   length,                            // Plain text length
                   true,                              // Encrypt
                   bufPtr                             // Add MIC at the end
    );

    // Update the length to include the MIC
    length += SECURITY_MIC_LENGTH;

    //-----------------------------
    // Prepare udp message

    // New udp message with no layer 2 security.
    msgSettings.mLinkSecurityEnabled = false;
    msgSettings.mPriority            = OT_MESSAGE_PRIORITY_NORMAL;
    udpMessage                       = otUdpNewMessage(&GetInstance(), &msgSettings);
    VerifyOrExit(udpMessage != NULL, OT_NOOP);

    // Add the security suite
    securitySuite = HEADER_SEC_SUITE_ENABLE;
    error         = otMessageAppend(udpMessage, &securitySuite, sizeof(uint8_t));
    VerifyOrExit(error == OT_ERROR_NONE, OT_NOOP);

    // Append the udp content (security header + encrypted payload + MIC)
    bufPtr = &buffer[2 * sizeof(otIp6Address)];
    error  = otMessageAppend(udpMessage, bufPtr, length + SECURITY_HEADER_LENGTH);
    VerifyOrExit(error == OT_ERROR_NONE, OT_NOOP);

    //-----------------------------
    // Send out the udp packet

    error = otUdpSendDatagram(&GetInstance(), udpMessage, &msgInfo);
    VerifyOrExit(error == OT_ERROR_NONE, OT_NOOP);

    udpMessage = NULL;
    IncrementFrameCounter();

exit:
    // If an error occurs before the udpMessage could be sent
    // then free it here to avoid a leak.
    if (udpMessage != NULL)
    {
        otMessageFree(udpMessage);
    }

    return;
}

uint8_t *LegacyPairingExtension::AppendTlvs(uint8_t *aBufPtr, uint8_t aCommand, const uint8_t *aResponse)
{
    // Mode tlv
    switch (aCommand)
    {
    case MLE_CMD_LINK_REQUEST:
    case MLE_CMD_LINK_ACCEPT:
    case MLE_CMD_LINK_ACCEPT_REQUEST:
    case MLE_CMD_ADVERTISEMENT:

        *aBufPtr++ = TLV_MODE_TYPE;
        *aBufPtr++ = TLV_MODE_LENGTH;
        *aBufPtr++ = TLV_MODE_VALUE;
        break;
    }

    // Response tlv
    switch (aCommand)
    {
    case MLE_CMD_LINK_ACCEPT:
    case MLE_CMD_LINK_ACCEPT_REQUEST:

        VerifyOrExit(aResponse != NULL, OT_NOOP);
        *aBufPtr++ = TLV_RESPONSE_TYPE;
        *aBufPtr++ = TLV_RESPONSE_LENGTH;
        memcpy(aBufPtr, aResponse, TLV_RESPONSE_LENGTH);
        aBufPtr += TLV_RESPONSE_LENGTH;
        break;
    }

    // Challenge tlv
    switch (aCommand)
    {
    case MLE_CMD_LINK_REQUEST:
    case MLE_CMD_LINK_ACCEPT_REQUEST:
    case MLE_CMD_LINK_ACCEPT:

        *aBufPtr++ = TLV_CHALLENGE_TYPE;
        *aBufPtr++ = TLV_CHALLENGE_LENGTH;
        memcpy(aBufPtr, mChallenge, TLV_RESPONSE_LENGTH);
        aBufPtr += TLV_CHALLENGE_LENGTH;
        break;
    }

    // Link-layer frame counter tlv
    switch (aCommand)
    {
    case MLE_CMD_LINK_ACCEPT_REQUEST:
    case MLE_CMD_LINK_ACCEPT:
    {
        uint32_t frameCounter = GetFrameCounter();

        *aBufPtr++ = TLV_LL_FRAME_COUNTER_TYPE;
        *aBufPtr++ = TLV_LL_FRAME_COUNTER_LENGTH;
        // Big-endian
        aBufPtr[0] = (frameCounter >> 24) & 0xff;
        aBufPtr[1] = (frameCounter >> 16) & 0xff;
        aBufPtr[2] = (frameCounter >> 8) & 0xff;
        aBufPtr[3] = (frameCounter >> 0) & 0xff;
        aBufPtr += TLV_LL_FRAME_COUNTER_LENGTH;
    }
    break;
    }

    // Only include the legacy ula tlv if we have valid one.
    if (IsUlaPrefixValid(mLegacyUlaPrefix))
    {
        // Ula prefix tlv
        switch (aCommand)
        {
        case MLE_CMD_LINK_ACCEPT_REQUEST:
        case MLE_CMD_LINK_ACCEPT:

            *aBufPtr++ = TLV_ULA_PREFIX_TYPE;
            *aBufPtr++ = TLV_ULA_PREFIX_LENGTH;
            memcpy(aBufPtr, mLegacyUlaPrefix, TLV_ULA_PREFIX_LENGTH);
            aBufPtr += TLV_ULA_PREFIX_LENGTH;
            break;
        }
    }

    // Timeout tlv
    switch (aCommand)
    {
    case MLE_CMD_LINK_REQUEST:

        *aBufPtr++ = TLV_TIMEOUT_TYPE;
        *aBufPtr++ = TLV_TIMEOUT_LENGTH;
        // Big-endian
        aBufPtr[0] = (TLV_TIMEOUT_VALUE_DEFAULT >> 24) & 0xff;
        aBufPtr[1] = (TLV_TIMEOUT_VALUE_DEFAULT >> 16) & 0xff;
        aBufPtr[2] = (TLV_TIMEOUT_VALUE_DEFAULT >> 8) & 0xff;
        aBufPtr[3] = (TLV_TIMEOUT_VALUE_DEFAULT >> 0) & 0xff;
        aBufPtr += TLV_TIMEOUT_LENGTH;
        break;
    }

exit:
    return aBufPtr;
}

void LegacyPairingExtension::SendAdvertisment(void)
{
    otIp6Address dstIpAddress;

    SetIp6AddressToLinkLocalMulticastAllNodes(&dstIpAddress);
    SendMleCommand(MLE_CMD_ADVERTISEMENT, NULL, &dstIpAddress);
}

void LegacyPairingExtension::HandleTimer(Timer &aTimer)
{
    static_cast<LegacyPairingExtension &>(aTimer.GetOwner<ExtensionBase>()).HandleTimer();
}

void LegacyPairingExtension::HandleTimer(void)
{
    uint32_t timerPeriod = 0;

    switch (mState)
    {
    case kStateJoining:

        mJoinAttemptsCounter++;
        if (mJoinAttemptsCounter <= MLE_MAX_JOIN_ATTEMPTS)
        {
            Join(NULL);
            timerPeriod = MLE_JOIN_TX_PERIOD_IN_MS;
            break;
        }

        // Fall through

    case kStateEnabled:
        mState = kStateEnabled;

        SendAdvertisment();
        timerPeriod = MLE_ADVR_TX_PERIOD_IN_MS;
        break;

    case kStateDisabled:
        break;
    }

    if (timerPeriod)
    {
        mTimer.Start(timerPeriod);
    }
}

} // namespace Extension
} // namespace ot

void otLegacyInit(void)
{
    ot::Extension::LegacyPairingExtension::InitLegacy();
}

void otLegacyStart(void)
{
    ot::Extension::LegacyPairingExtension::StartLegacy();
}

void otLegacyStop(void)
{
    ot::Extension::LegacyPairingExtension::StopLegacy();
}

void otSetLegacyUlaPrefix(const uint8_t *aUlaPrefix)
{
    ot::Extension::LegacyPairingExtension::SetLegacyUlaPrefix(aUlaPrefix);
}
