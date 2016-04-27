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
 *   This file includes definitions for IPv6 packet processing.
 */

#ifndef IP6_HPP_
#define IP6_HPP_

#include <stddef.h>

#include <common/encoding.hpp>
#include <common/message.hpp>
#include <common/thread_error.hpp>
#include <net/ip6_address.hpp>
#include <net/netif.hpp>
#include <net/socket.hpp>

using Thread::Encoding::BigEndian::HostSwap16;

namespace Thread {

/**
 * @namespace Ip6
 *
 * @brief
 *   This namespace includes definitions for IPv6 networking.
 *
 */
namespace Ip6 {

/**
 * @addtogroup core-ipv6
 *
 * @brief
 *   This module includes definitions for the IPv6 network layer.
 *
 * @{
 *
 * @defgroup core-ip6-icmp6 ICMPv6
 * @defgroup core-ip6-ip6 IPv6
 * @defgroup core-ip6-mpl MPL
 * @defgroup core-ip6-netif Network Interfaces
 *
 * @}
 *
 */

/**
 * @addtogroup core-ip6-ip6
 *
 * @brief
 *   This module includes definitions for core IPv6 networking.
 *
 * @{
 *
 */

/**
 * Internet Protocol Numbers
 */
enum IpProto
{
    kProtoHopOpts  = 0,   ///< IPv6 Hop-by-Hop Option
    kProtoUdp      = 17,  ///< User Datagram
    kProtoIp6      = 41,  ///< IPv6 encapsulation
    kProtoRouting  = 43,  ///< Routing Header for IPv6
    kProtoFragment = 44,  ///< Fragment Header for IPv6
    kProtoIcmp6    = 58,  ///< ICMP for IPv6
    kProtoNone     = 59,  ///< No Next Header for IPv6
    kProtoDstOpts  = 60,  ///< Destination Options for IPv6
};

/**
 * This class implements IPv6 header generation and parsing.
 *
 */
class Header
{
public:
    /**
     * This method initializes the IPv6 header.
     *
     */
    void Init() { mVersionClassFlow.m32[0] = 0; mVersionClassFlow.m8[0] = kVersion6; }

    /**
     * This method indicates whether or not the IPv6 Version is set to 6.
     *
     * @retval TRUE   If the IPv6 Version is set to 6.
     * @retval FALSE  If the IPv6 Version is not set to 6.
     *
     */
    bool IsVersion6() const { return (mVersionClassFlow.m8[0] & kVersionMask) == kVersion6; }

    /**
     * This method returns the IPv6 Payload Length value.
     *
     * @returns The IPv6 Payload Length value.
     *
     */
    uint16_t GetPayloadLength() { return HostSwap16(mPayloadLength); }

    /**
     * This method sets the IPv6 Payload Length value.
     *
     * @param[in]  aLength  The IPv6 Payload Length value.
     *
     */
    void SetPayloadLength(uint16_t aLength) { mPayloadLength = HostSwap16(aLength); }

    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader() const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the IPv6 Hop Limit value.
     *
     * @returns The IPv6 Hop Limit value.
     *
     */
    uint8_t GetHopLimit() const { return mHopLimit; }

    /**
     * This method sets the IPv6 Hop Limit value.
     *
     * @param[in]  aHopLimit  The IPv6 Hop Limit value.
     *
     */
    void SetHopLimit(uint8_t aHopLimit) { mHopLimit = aHopLimit; }

    /**
     * This method returns the IPv6 Source address.
     *
     * @returns A reference to the IPv6 Source address.
     *
     */
    Address &GetSource() { return mSource; }

    /**
     * This method sets the IPv6 Source address.
     *
     * @param[in]  aSource  A reference to the IPv6 Source address.
     *
     */
    void SetSource(const Address &aSource) { mSource = aSource; }

    /**
     * This method returns the IPv6 Destination address.
     *
     * @returns A reference to the IPv6 Destination address.
     *
     */
    Address &GetDestination() { return mDestination; }

    /**
     * This method sets the IPv6 Destination address.
     *
     * @param[in]  aDestination  A reference to the IPv6 Destination address.
     *
     */
    void SetDestination(const Address &aDestination) { mDestination = aDestination; }

    /**
     * This static method returns the byte offset of the IPv6 Payload Length field.
     *
     * @returns The byte offset of the IPv6 Payload Length field.
     *
     */
    static uint8_t GetPayloadLengthOffset() { return offsetof(Header, mPayloadLength); }

    /**
     * This static method returns the byte offset of the IPv6 Hop Limit field.
     *
     * @returns The byte offset of the IPv6 Hop Limit field.
     *
     */
    static uint8_t GetHopLimitOffset() { return offsetof(Header, mHopLimit); }

    /**
     * This static method returns the size of the IPv6 Hop Limit field.
     *
     * @returns The size of the IPv6 Hop Limit field.
     *
     */
    static uint8_t GetHopLimitSize() { return sizeof(mHopLimit); }

    /**
     * This static method returns the byte offset of the IPv6 Destination field.
     *
     * @returns The byte offset of the IPv6 Destination field.
     *
     */
    static uint8_t GetDestinationOffset() { return offsetof(Header, mDestination); }

private:
    enum
    {
        kVersion6 = 0x60,
        kVersionMask = 0xf0,
        kVersionClassFlowSize = 4,
    };

    union
    {
        uint8_t  m8[kVersionClassFlowSize / sizeof(uint8_t)];
        uint16_t m16[kVersionClassFlowSize / sizeof(uint16_t)];
        uint32_t m32[kVersionClassFlowSize / sizeof(uint32_t)];
    } mVersionClassFlow;
    uint16_t mPayloadLength;
    uint8_t  mNextHeader;
    uint8_t  mHopLimit;
    Address  mSource;
    Address  mDestination;
} __attribute__((packed));

class ExtensionHeader
{
public:
    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader() const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the IPv6 Header Extension Length value.
     *
     * @returns The IPv6 Header Extension Length value.
     *
     */
    uint16_t GetLength() const { return mLength; }

    /**
     * This method sets the IPv6 Header Extension Length value.
     *
     * @param[in]  aLength  The IPv6 Header Extension Length value.
     *
     */
    void SetLength(uint16_t aLength) { mLength = aLength; }

private:
    uint8_t mNextHeader;
    uint8_t mLength;
} __attribute__((packed));

/**
 * This class implements IPv6 Hop-by-Hop Options Header generation and parsing.
 *
 */
class HopByHopHeader: public ExtensionHeader
{
} __attribute__((packed));

/**
 * This class implements IPv6 Options generation and parsing.
 *
 */
class OptionHeader
{
public:
    /**
     * This method returns the IPv6 Option Type value.
     *
     * @returns The IPv6 Option Type value.
     *
     */
    uint8_t GetType() const { return mType; }

    /**
     * This method sets the IPv6 Option Type value.
     *
     * @param[in]  aType  The IPv6 Option Type value.
     *
     */
    void SetType(uint8_t aType) { mType = aType; }

    /**
     * IPv6 Option Type actions for unrecognized IPv6 Options.
     *
     */
    enum Action
    {
        kActionSkip      = 0x00,  ///< skip over this option and continue processing the header
        kActionDiscard   = 0x40,  ///< discard the packet
        kActionForceIcmp = 0x80,  ///< discard the packet and forcibly send an ICMP Parameter Problem
        kActionIcmp      = 0xc0,  ///< discard packet and conditionally send an ICMP Parameter Problem
        kActionMask      = 0xc0,  ///< mask for action bits
    };

    /**
     * This method returns the IPv6 Option action for unrecognized IPv6 Options.
     *
     * @returns The IPv6 Option action for unrecognized IPv6 Options.
     *
     */
    Action GetAction() const { return static_cast<Action>(mType & kActionMask); }

    /**
     * This method returns the IPv6 Option Length value.
     *
     * @returns The IPv6 Option Length value.
     *
     */
    uint8_t GetLength() const { return mLength; }

    /**
     * This method sets the IPv6 Option Length value.
     *
     * @param[in]  aLength  The IPv6 Option Length value.
     *
     */
    void SetLength(uint8_t aLength) { mLength = aLength; }

private:
    uint8_t mType;
    uint8_t mLength;
} __attribute__((packed));

/**
 * This class implements IPv6 Fragment Header generation and parsing.
 *
 */
class FragmentHeader
{
public:
    /**
     * This method initializes the IPv6 Fragment header.
     *
     */
    void Init() { mReserved = 0; mIdentification = 0; }

    /**
     * This method returns the IPv6 Next Header value.
     *
     * @returns The IPv6 Next Header value.
     *
     */
    IpProto GetNextHeader() const { return static_cast<IpProto>(mNextHeader); }

    /**
     * This method sets the IPv6 Next Header value.
     *
     * @param[in]  aNextHeader  The IPv6 Next Header value.
     *
     */
    void SetNextHeader(IpProto aNextHeader) { mNextHeader = static_cast<uint8_t>(aNextHeader); }

    /**
     * This method returns the Fragment Offset value.
     *
     * @returns The Fragment Offset value.
     *
     */
    uint16_t GetOffset() { return (HostSwap16(mOffsetMore) & kOffsetMask) >> kOffsetOffset; }

    /**
     * This method sets the Fragment Offset value.
     *
     * @param[in]  aOffset  The Fragment Offset value.
     */
    void SetOffset(uint16_t aOffset) {
        mOffsetMore = HostSwap16((HostSwap16(mOffsetMore) & kOffsetMask) | (aOffset << kOffsetOffset));
    }

    /**
     * This method returns the M flag value.
     *
     * @returns The M flag value.
     *
     */
    bool IsMoreFlagSet() { return HostSwap16(mOffsetMore) & kMoreFlag; }

    /**
     * This method clears the M flag value.
     *
     */
    void ClearMoreFlag() { mOffsetMore = HostSwap16(HostSwap16(mOffsetMore) & ~kMoreFlag); }

    /**
     * This method sets the M flag value.
     *
     */
    void SetMoreFlag() { mOffsetMore = HostSwap16(HostSwap16(mOffsetMore) | kMoreFlag); }

private:
    uint8_t mNextHeader;
    uint8_t mReserved;

    enum
    {
        kOffsetOffset = 3,
        kOffsetMask = 0xfff8,
        kMoreFlag = 1,
    };
    uint16_t mOffsetMore;
    uint32_t mIdentification;
} __attribute__((packed));

/**
 * This class implements the core IPv6 message processing.
 *
 */
class Ip6
{
public:
    enum
    {
        kDefaultHopLimit = 64,
        kMaxDatagramLength = 1500,
    };

    /**
     * This static method allocates a new message buffer from the buffer pool.
     *
     * @param[in]  aReserved  The number of header bytes to reserve following the IPv6 header.
     *
     * @returns A pointer to the message or NULL if insufficient message buffers are available.
     *
     */
    static Message *NewMessage(uint16_t aReserved);

    /**
     * This static method sends an IPv6 datagram.
     *
     * @param[in]  aMessage      A reference to the message.
     * @param[in]  aMessageInfo  A reference to the message info associated with @p aMessage.
     * @param[in]  aIpProto      The Internet Protocol value.
     *
     * @retval kThreadError_None    Successfully enqueued the message into an output interface.
     * @retval kThreadError_NoBufs  Insufficient available buffer to add the IPv6 headers.
     *
     */
    static ThreadError SendDatagram(Message &aMessage, MessageInfo &aMessageInfo, IpProto aIpProto);

    /**
     * This static method processes a received IPv6 datagram.
     *
     * @param[in]  aMessage          A reference to the message.
     * @param[in]  aNetif            A pointer to the network interface that received the message.
     * @param[in]  aInterfaceId      The interface identier of the network interface that received the message.
     * @param[in]  aLinkMessageInfo  A pointer to link-specific message information.
     * @param[in]  aFromNcpHost      TRUE if the message was submited by the NCP host, FALSE otherwise.
     *
     * @retval kThreadError_None   Successfully processed the message.
     * @retval kThreadError_Drop   Message processing failed and the message should be dropped.
     *
     */
    static ThreadError HandleDatagram(Message &aMessage, Netif *aNetif, uint8_t aInterfaceId,
                                      const void *aLinkMessageInfo, bool aFromNcpHost);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aValue     The 16-bit value to update @p aChecksum with.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, uint16_t aValue);


    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aBuf       A pointer to a buffer.
     * @param[in]  aLength    The number of bytes in @p aBuf.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, const void *aBuf, uint16_t aLength);

    /**
     * This static method updates a checksum.
     *
     * @param[in]  aChecksum  The checksum value to update.
     * @param[in]  aAddress   A reference to an IPv6 address.
     *
     * @returns The updated checksum.
     *
     */
    static uint16_t UpdateChecksum(uint16_t aChecksum, const Address &aAddress);

    /**
     * This static method computes the pseudoheader checksum.
     *
     * @param[in]  aSource       A reference to the IPv6 source address.
     * @param[in]  aDestination  A reference to the IPv6 destination address.
     * @param[in]  aLength       The IPv6 Payload Length value.
     * @param[in]  aProto        The IPv6 Next Header value.
     *
     * @returns The psuedoheader checksum.
     *
     */
    static uint16_t ComputePseudoheaderChecksum(const Address &aSource, const Address &aDestination,
                                                uint16_t aLength, IpProto aProto);

    /**
     * This function pointer is called when an IPv6 message is received.
     *
     * @param[in]  aContext  A pointer to arbitrary context information.
     * @param[in]  aMessage  A reference to the IPv6 message.
     *
     */
    typedef void (*NcpReceivedDatagramHandler)(void *aContext, Message &aMessage);

    /**
     * This method sets the NCP receive handler.
     *
     * @param[in]  aHandler  A pointer to a function that is called when IPv6 messages are received.
     * @param[in]  aContext  A pointer to arbitrary context information.
     *
     */
    static void SetNcpReceivedHandler(NcpReceivedDatagramHandler handler, void *context);
};

/**
 * @}
 *
 */

}  // namespace Ip6
}  // namespace Thread

#endif  // NET_IP6_HPP_
