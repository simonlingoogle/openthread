/*
 *  Copyright (c) 2017, The OpenThread Authors.
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
 * @brief
 *  This file defines the top-level DNS functions for the OpenThread library.
 */

#ifndef OPENTHREAD_DNS_H_
#define OPENTHREAD_DNS_H_

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/ip6.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup api-dns
 *
 * @brief
 *   This module includes functions that control DNS communication.
 *
 * @{
 *
 */

#define OT_DNS_MAX_NAME_SIZE 255 ///< Maximum name string size (includes null char at the end of string).

#define OT_DNS_MAX_LABEL_SIZE 64 ///< Maximum label string size (include null char at the end of string).

#define OT_DNS_TXT_KEY_MIN_LENGTH 1 ///< Minimum length of TXT record key string (RFC 6763 - section 6.4).

#define OT_DNS_TXT_KEY_MAX_LENGTH 9 ///< Recommended maximum length of TXT record key string (RFC 6763 - section 6.4).

/**
 * This structure represents a TXT record entry representing a key/value pair (RFC 6763 - section 6.3).
 *
 * The string buffers pointed to by `mKey` and `mValue` MUST persist and remain unchanged after an instance of such
 * structure is passed to OpenThread (as part of `otSrpClientService` instance).
 *
 * An array of `otDnsTxtEntry` entries are used in `otSrpClientService` to specify the full TXT record (a list of
 * entries).
 *
 */
typedef struct otDnsTxtEntry
{
    /**
     * The TXT record key string.
     *
     * If `mKey` is not NULL, then it MUST be a null-terminated C string. The entry is treated as key/value pair with
     * `mValue` buffer providing the value.
     *   - The entry is encoded as follows:
     *        - A single string length byte followed by "key=value" format (without the quotation marks).
              - In this case, the overall encoded length must be 255 bytes or less.
     *   - If `mValue` is NULL, then key is treated as a boolean attribute and encoded as "key" (with no `=`).
     *   - If `mValue` is not NULL but `mValueLength` is zero, then it is treated as empty value and encoded as "key=".
     *
     * If `mKey` is NULL, then `mValue` buffer is treated as an already encoded TXT-DATA and is appended as is in the
     * DNS message.
     *
     */
    const char *   mKey;
    const uint8_t *mValue;       ///< The TXT record value or already encoded TXT-DATA (depending on `mKey`).
    uint16_t       mValueLength; ///< Number of bytes in `mValue` buffer.
} otDnsTxtEntry;

/**
 * This structure represents an iterator for TXT record entires (key/value pairs).
 *
 * The data fields in this structure are intended for use by OpenThread core and caller should not read or change them.
 *
 */
typedef struct otDnsTxtEntryIterator
{
    const void *mPtr;
    uint16_t    mData[2];
    char        mChar[OT_DNS_TXT_KEY_MAX_LENGTH + 1];
} otDnsTxtEntryIterator;

/**
 * This function initializes a TXT record iterator.
 *
 * The buffer pointer @p aTxtData and its content MUST persist and remain unchanged while @p aIterator object
 * is being used.
 *
 * @param[in] aIterator       A pointer to the iterator to initialize (MUST NOT be NULL).
 * @param[in] aTxtData        A pointer to buffer containing the encoded TXT data.
 * @param[in] aTxtDataLength  The length (number of bytes) of @p aTxtData.
 *
 */
void otDnsInitTxtEntryIterator(otDnsTxtEntryIterator *aIterator, const uint8_t *aTxtData, uint16_t aTxtDataLength);

/**
 * This function parses the TXT data from an iterator and gets the next TXT record entry (key/value pair).
 *
 * The @p aIterator MUST be initialized using `otDnsInitTxtEntryIterator()` before calling this function and the TXT
 * data buffer used to initialize the iterator MUST persist and remain unchanged. Otherwise the behavior of this
 * function is undefined.
 *
 * If the parsed key string length is smaller than or equal to `OT_DNS_TXT_KEY_MAX_LENGTH` (recommended max key length)
 * the key string is returned in `mKey` in @p aEntry. But if the key is longer, then `mKey` is set to NULL and the
 * entire encoded TXT entry string is returned in `mValue` and `mValueLength`.
 *
 * @param[in]  aIterator   A pointer to the iterator (MUST NOT be NULL).
 * @param[out] aEntry      A pointer to a `otDnsTxtEntry` structure to output the parsed/read entry (MUST NOT be NULL).
 *
 * @retval OT_ERROR_NONE       The next entry was parsed successfully. @p aEntry is updated.
 * @retval OT_ERROR_NOT_FOUND  No more entries in the TXT data.
 * @retval OT_ERROR_PARSE      The TXT data from @p aIterator is not well-formed.
 *
 */
otError otDnsGetNextTxtEntry(otDnsTxtEntryIterator *aIterator, otDnsTxtEntry *aEntry);

/**
 * This function is called when a DNS-SD query subscribes a service or service instance.
 *
 * @param[in] aContext      A pointer to the application-specific context.
 * @param[in] aFullName     The null-terminated full service name (e.g. "_ipps._tcp.default.service.arpa."), or
 *                          full service instance name (e.g. "OpenThread._ipps._tcp.default.service.arpa.").
 *
 */
typedef void (*otDnssdQuerySubscribe)(void *aContext, const char *aFullName);

/**
 * This function is called when a DNS-SD query unsubscribes a service or service instance.
 *
 * @param[in] aContext      A pointer to the application-specific context.
 * @param[in] aFullName     The null-terminated full service name (e.g. "_ipps._tcp.default.service.arpa."), or
 *                          full service instance name (e.g. "OpenThread._ipps._tcp.default.service.arpa.").
 *
 */
typedef void (*otDnssdQueryUnsubscribe)(void *aContext, const char *aFullName);

/**
 * This structure represents information of a discovered service instance for a DNS-SD query.
 *
 */
typedef struct otDnssdServiceInstanceInfo
{
    const char *   mFullName;  ///< Full instance name (e.g. "OpenThread._ipps._tcp.default.service.arpa.").
    const char *   mHostName;  ///< Host name.
    otIp6Address   mAddress;   ///< Host IPv6 address.
    uint16_t       mPort;      ///< Service port.
    uint16_t       mPriority;  ///< Service priority.
    uint16_t       mWeight;    ///< Service weight.
    uint16_t       mTxtLength; ///< Service TXT RDATA length.
    const uint8_t *mTxtData;   ///< Service TXT RDATA.
    uint32_t       mTtl;       ///< Service TTL (in seconds).
} otDnssdServiceInstanceInfo;

/**
 * This function sets DNS-SD server query callbacks. The DNS-SD server calls @p aSubscribe to subscribe to a service or
 * service instance to resolve a DNS-SD query and @p aUnsubscribe to unsubscribe when the query is resolved or timeout.
 *
 * @note @p aSubscribe and @p aUnsubscribe should be both set or unset.
 *
 * @param[in] aInstance     The OpenThread instance structure.
 * @param[in] aContext      A pointer to the application-specific context.
 * @param[in] aSubscribe    A pointer to the callback function to subscribe a service or service instance.
 * @param[in] aUnsubscribe  A pointer to the callback function to unsubscribe a service or service instance.
 *
 */
void otDnssdQuerySetCallbacks(otInstance *            aInstance,
                              void *                  aContext,
                              otDnssdQuerySubscribe   aSubscribe,
                              otDnssdQueryUnsubscribe aUnsubscribe);

/**
 * This function notifies a discovered service instance. The external query resolver (e.g. Discovery Proxy) should call
 * this function to notify OpenThread core of the subscribed services or service instances.
 *
 * @param[in] aInstance         The OpenThread instance structure.
 * @param[in] aServiceFullName  The null-terminated full service name.
 * @param[in] aInstanceInfo     A pointer to the discovered service instance information.
 *
 */
void otDnssdQueryNotifyServiceInstance(otInstance *                aInstance,
                                       const char *                aServiceFullName,
                                       otDnssdServiceInstanceInfo *aInstanceInfo);

/**
 * @}
 *
 */

#ifdef __cplusplus
} // extern "C"
#endif

#endif // OPENTHREAD_DNS_H_
