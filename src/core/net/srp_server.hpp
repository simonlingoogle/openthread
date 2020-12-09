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
 *   This file includes definitions for SRP server.
 */

#ifndef BORDER_ROUTER_SRP_SERVER_HPP_
#define BORDER_ROUTER_SRP_SERVER_HPP_

#include "openthread-core-config.h"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#include <openthread/ip6.h>
#include <openthread/srp_server.h>

#include "common/clearable.hpp"
#include "common/linked_list.hpp"
#include "common/locator.hpp"
#include "common/non_copyable.hpp"
#include "common/notifier.hpp"
#include "common/timer.hpp"
#include "crypto/ecdsa.hpp"
#include "net/dns_headers.hpp"
#include "net/ip6.hpp"
#include "net/ip6_address.hpp"
#include "net/udp6.hpp"

namespace ot {

namespace Srp {

/**
 * This class implements the SRP server.
 *
 */
class Server : public InstanceLocator, private NonCopyable
{
    friend class ot::Notifier;

public:
    class Host;
    class Service;

    enum : uint8_t
    {
        kMaxAddressesNum = 8, ///< The maximum number of addresses for a host.
    };

    /**
     * This class represents a server-side SRP service.
     *
     */
    class Service : public otSrpServerService, public LinkedListEntry<Service>, private NonCopyable
    {
        friend class Server;

    public:
        /**
         * This method creates a new Service object with given full name.
         *
         * @param[in]  aFullName  The full name of the service instance.
         *
         * @returns  A pointer to the newly created Service object. nullptr if
         *           cannot allocate memory for the object.
         *
         */
        static Service *New(const char *aFullName);

        /**
         * This method destroys a Service object.
         *
         * @param[in]  aService  The Service object to be destroyed.
         *
         */
        static void Destroy(Service *aService);

        /**
         * This method tells whether this Service object has been deleted.
         *
         * The Service object retains event if the service instance has been deleted by the SRP client,
         * because the service instance name may retain.
         *
         * @returns  A boolean that indicates whether the service has been deleted.
         *
         */
        bool IsDeleted(void) const { return mIsDeleted; }

        /**
         * This method returns the expire time (in milliseconds) of the service.
         *
         * @returns  The service expire time in milliseconds.
         *
         */
        TimeMilli GetExpireTime(void) const;

        /**
         * This method returns the expire time (in milliseconds) of the key associated to the service.
         *
         * @returns  The service key expire time in milliseconds.
         *
         */
        TimeMilli GetKeyExpireTime(void) const;

        /**
         * This method returns the Host associated with this service.
         *
         * @returns  A reference to the host.
         *
         */
        const Host &GetHost(void) const { return *static_cast<Host *>(mHost); }

        /**
         * This method returns the full service name.
         *
         * @returns  A pointer to the null-terminated full name string.
         *
         */
        const char *GetFullName(void) const { return mFullName; }

        /**
         * This method tells whether this service matches to a given full service name.
         *
         * @param[in]  aFullName  The full service name.
         *
         * @returns  A boolean that indicates whether this service matches the given name.
         *
         */
        bool Matches(const char *aFullName) const;

    private:
        Service();
        ~Service();

        /**
         * This method deletes but retains the service instance.
         *
         * @param[in]  aDeleted  A boolean that indicates whether the service should be deleted or not.
         *
         */
        void  SetDeleted(bool aDeleted) { mIsDeleted = aDeleted; }
        Host &GetHost(void) { return *static_cast<Host *>(mHost); }
        char *GetFullName(void) { return mFullName; }

        /**
         * This method sets the full service name.
         *
         * @param[in]  aFullName  The full service name.
         *
         * @retval  OT_ERROR_NONE     Successfully set the full service name.
         * @retval  OT_ERROR_NO_BUFS  Failed to allocate memory for the name.
         *
         */
        otError SetFullName(const char *aFullName);

        /**
         * This method sets the TXT data.
         *
         * @param[in]  aTxtData        A pointer to the TXT data.
         * @param[in]  aTxtDataLength  The length of the TXT data in bytes.
         *
         * @retval OT_ERROR_NONE     Successfully set the TXT data.
         * @retval OT_ERROR_NO_BUFS  Failed to allocate memory for the TXT data.
         *
         */
        otError SetTxtData(const uint8_t *aTxtData, uint16_t aTxtDataLength);

        /**
         * This method reads and sets TXT data from a message.
         *
         * @param[in]  aMessage  The message that includes the TXT data.
         * @param[in]  aOffset   The offset from which TXT data starts.
         * @param[in]  aLength   The length of the TXT data.
         *
         * @retval  OT_ERROR_NONE    Successfully read and set the TXT data from the message.
         * @retval  OT_ERROR_PARSE   Failed to read the TXT data from the message.
         * @retval OT_ERROR_NO_BUFS  Failed to allocate memory for the TXT data.
         *
         */
        otError SetTxtDataFromMessage(const Message &aMessage, uint16_t aOffset, uint16_t aLength);

        /**
         * This method clears all resources associated to this service.
         *
         */
        void ClearResources(void);

        /**
         * This method copies all resources from another service.
         *
         * @param[in]  aService  The service to copies resource from.
         *
         * @retval  OT_ERROR_NONE     Successfully copied all resources.
         * @retval  OT_ERROR_NO_BUFS  Failed to allocate memory for those resources.
         *
         */
        otError CopyResourcesFrom(const Service &aService);

        TimeMilli mTimeLastUpdate;
    };

    /**
     * This class represents a Host which registers one or more services on this SRP server.
     *
     */
    class Host : public LinkedListEntry<Host>, private NonCopyable
    {
    public:
        friend class LinkedListEntry<Host>;

        /**
         * This method creates a new Host object with given full name.
         *
         * @returns  A pointer to the newly created Host object. nullptr if
         *           cannot allocate memory for the object.
         *
         */
        static Host *New(void);

        /**
         * This method destroys a Host object.
         *
         * @param[in]  aHost  A pointer to the host to be destroyed.
         *
         */
        static void Destroy(Host *aHost);

        /**
         * This method tells whether this Host object has been deleted.
         *
         * The Host object retains event if the host has been deleted by the SRP client,
         * because the host name may retain.
         *
         * @returns  A boolean that indicates whether the host has been deleted.
         *
         */
        bool IsDeleted(void) const { return (mLease == 0); }

        /**
         * This method returns the full host name.
         *
         * @returns  A pointer to the null-terminated full host name.
         *
         */
        const char *GetFullName(void) const { return mFullName; }
        char *      GetFullName(void) { return mFullName; }

        /**
         * This method sets the full name for this host.
         *
         * @param[in]  aFullName  A pointer to the null-terminated full name.
         *
         * @retval  OT_ERROR_NONE     Successfully set the name.
         * @retval  OT_ERROR_NO_BUFS  Failed to allocate memory for the name.
         *
         */
        otError SetFullName(const char *aFullName);

        /**
         * This method returns adrersses of this host.
         *
         * @param[out]  aAddressesNum  The number of the address.
         *
         * @returns  A pointer to the addresses array.
         *
         */
        const Ip6::Address *GetAddresses(uint8_t &aAddressesNum) const
        {
            aAddressesNum = mAddressesNum;
            return mAddresses;
        }

        /**
         * This method sets the KEY resource for this host.
         *
         * @param[in]  aKey  The ECDSA P-256 public key.
         *
         */
        void SetKey(Dns::Ecdsa256KeyRecord &aKey);

        /**
         * This method sets the LEASE time value for the host and all its services.
         *
         * @param[in]  aLease  The LEASE in seconds.
         *
         */
        void SetLease(uint32_t aLease);

        /**
         * This method returns the LEASE time value of the host.
         *
         * @returns  The LEASE time value in seconds.
         *
         */
        uint32_t GetLease(void) const { return mLease; }

        /**
         * This method sets the KEY-LEASE time value for the key of the host and all its services.
         *
         * @param[in]  aKeyLease  The KEY-LEASE in seconds.
         *
         */
        void SetKeyLease(uint32_t aKeyLease);

        /**
         * This method returns the KEY-LEASE time value of the key of the host
         *
         * @returns  The KEY-LEASE time value in seconds.
         *
         */
        uint32_t GetKeyLease(void) const { return mKeyLease; }

        /**
         * This method returns the KEY resource of the host.
         *
         * @returns  A pointer to the ECDSA P 256 public key if there is valid one. Otherwise, nullptr.
         *
         */
        const Dns::Ecdsa256KeyRecord *GetKey(void) const { return mKey.IsValid() ? &mKey : nullptr; }

        /**
         * This method returns the expire time (in milliseconds) of the host.
         *
         * @returns  The expire time in milliseconds.
         *
         */
        TimeMilli GetExpireTime(void) const;

        /**
         * This method returns the expire time (in milliseconds) of the key of the host.
         *
         * @returns  The expire time of the key in milliseconds.
         *
         */
        TimeMilli GetKeyExpireTime(void) const;

        /**
         * This method returns the list of all services associated with this host.
         *
         * @returns  A pointer to the first service.
         *
         */
        const Service *GetServices(void) const { return mServices.GetHead(); }
        Service *      GetServices(void) { return mServices.GetHead(); }

        /**
         * This method adds a service to this host, do nothing if a service with the same name already exists.
         *
         * @param[in]  aFullName  The full name of the service.
         *
         * @returns  A pointer to the newly added service or existing service which has the same name. nullptr if
         *           failed to allocate memory for the new service.
         *
         */
        Service *AddService(const char *aFullName);

        /**
         * This method adds a service to this host.
         *
         * @param[in]  aService  The service to be added.
         *
         */
        void AddService(Service *aService);

        /**
         * This method removes a service from this host.
         *
         * @param[in]  aService  The service to be removed.
         *
         */
        void RemoveService(Service *aService);

        /**
         * This method removes and destroys all services from this host.
         *
         */
        void RemoveAllServices(void);

        /**
         * This method clears resources of this host.
         *
         */
        void ClearResources(void);

        /**
         * This method copies resources from another host.
         *
         * @param[in]  aHost  The host to copy resources from.
         *
         */
        void CopyResourcesFrom(const Host &aHost);

        /**
         * This method returns the service with given full name.
         *
         * @param[in]  aFullName  The full name of the service.
         *
         * @returns  A pointer to the matched service if there exists. Otherwise, nullptr.
         *
         */
        Service *FindService(const char *aFullName);

        /**
         * This method adds an IPv6 address to this host.
         *
         * @param[in]  aIp6Address  The IPv6 address to be added.
         *
         * @retval  OT_ERROR_NONE     Successfully added the IPv6 address.
         * @retval  OT_ERROR_DROP     The address has been dropped.
         * @retval  OT_ERROR_NO_BUFS  There is no space for the IPv6 address.
         *
         */
        otError AddIp6Address(const Ip6::Address &aIp6Address);

        /**
         * This method tells whether this host matches to a given full host name.
         *
         * @param[in]  aFullName  The full host name.
         *
         * @returns  A boolean that indicates whether this host matches to the given name.
         *
         */
        bool Matches(const char *aName) const;

    private:
        Host(void);
        ~Host(void);

        char *       mFullName;                    // The full hostname.
        Ip6::Address mAddresses[kMaxAddressesNum]; // The address list to be advertised.
        uint8_t      mAddressesNum;                // The number of addresses.
        Host *       mNext;                        // The pointer to the next host.

        Dns::Ecdsa256KeyRecord mKey;            // The KEY resource.
        uint32_t               mLease;          // The LEASE time in seconds.
        uint32_t               mKeyLease;       // The LEASE time of the KEY in seconds.
        TimeMilli              mTimeLastUpdate; // The time of the last update to this host.
        LinkedList<Service>    mServices;       // The service list.
    };

    /**
     * This constructor initializes the SRP server object.
     *
     * @param[in]  aInstance  A reference to the OpenThread instance.
     *
     */
    explicit Server(Instance &aInstance);

    /**
     * This method sets the SRP service events handler.
     *
     * @param[in]  aServiceHandler         A service events handler.
     * @param[in]  aServiceHandlerContext  A pointer to arbitrary context information.
     *
     * @note  The handler SHOULD call HandleAdvertisingResult to report the result of its processing.
     *        Otherwise, a SRP service will be considered failed.
     *
     * @sa  HandleAdvertisingResult
     *
     */
    void SetServiceHandler(otSrpServerAdvertisingHandler aServiceHandler, void *aServiceHandlerContext);

    /**
     * This method tells whether the SRP server is currently running.
     *
     * @returns  A boolean that indicates whether the server is running.
     *
     */
    bool IsRunning(void) const;

    /**
     * This method enables/disables the SRP server.
     *
     * @param[in]  aEnabled  A boolean to enable/disable the SRP server.
     *
     */
    void SetEnabled(bool aEnabled);

    /**
     * This method sets the LEASE & KEY-LEASE ranges that are acceptable for the SRP server.
     *
     * @param[in]  aMinLease     The minimum LEASE in seconds.
     * @param[in]  aMaxLease     The maximum LEASE in seconds.
     * @param[in]  aMinKeyLease  The minimum KEY-LEASE in seconds.
     * @param[in]  aMaxKeyLease  The maximum KEY-LEASE in seconds.
     *
     * @retval  OT_ERROR_NONE          Successfully set the LEASE and KEY-LEASE ranges.
     * @retval  OT_ERROR_INVALID_ARGS  The LEASE or KEY-LEASE range is not valid.
     *
     */
    otError SetLeaseRange(uint32_t aMinLease, uint32_t aMaxLease, uint32_t aMinKeyLease, uint32_t aMaxKeyLease);

    /**
     * This method returns the next SRP host registered on this SRP server.
     *
     * @param[in]  aHost  The current SRP host. Use nullptr to get the first SRP host.
     *
     * @returns  A pointer to the next SRP host; nullptr, if no more SRP host can be found.
     *
     */
    const Host *GetNextHost(const Host *aHost);

    /**
     * This method receives the service advertising result.
     *
     * @param[in]  aHost   A pointer to the Host object which contains the SRP service updates.
     * @param[in]  aError  The service advertising result.
     *
     */
    void HandleAdvertisingResult(const Host *aHost, otError aError);

private:
    enum : uint8_t
    {
        kThreadServiceTypeSrpServer = 0x5du, // The temporary Thread service type for SRP server.
    };

    enum : uint16_t
    {
        kUdpPayloadSize = Ip6::Ip6::kMaxDatagramLength - sizeof(Ip6::Udp::Header), // Max UDP payload size
    };

    enum : uint32_t
    {
        kDefaultMinLease             = 60u * 30,        // Default minimum lease time, 30 min (in seconds).
        kDefaultMaxLease             = 3600u * 2,       // Default maximum lease time, 2 hours (in seconds).
        kDefaultMinKeyLease          = 3600u * 24,      // Default minimum key-lease time, 1 day (in seconds).
        kDefaultMaxKeyLease          = 3600u * 24 * 14, // Default maximum key-lease time, 14 days (in seconds).
        kDefaultEventsHandlerTimeout = 500u,            // Default events handler timeout (in milliseconds).
    };

    /**
     * This class includes metadata for processing a SRP update (register, deregister)
     * and sending DNS response to the client.
     *
     */
    class UpdateMetadata : public LinkedListEntry<UpdateMetadata>
    {
    public:
        friend class LinkedListEntry<UpdateMetadata>;

        static UpdateMetadata *New(const Dns::UpdateHeader &aHeader, Host *aHost, const Ip6::MessageInfo &aMessageInfo);
        static void            Destroy(UpdateMetadata *aUpdateMetadata);
        TimeMilli              GetExpireTime(void) const { return mExpireTime; }
        const Dns::UpdateHeader &GetDnsHeader(void) const { return mDnsHeader; }
        Host &                   GetHost(void) { return *mHost; }
        const Ip6::MessageInfo & GetMessageInfo(void) const { return mMessageInfo; }
        bool Matches(uint16_t aDnsMessageId) const { return mDnsHeader.GetMessageId() == aDnsMessageId; }
        bool Matches(const Host *aHost) const { return mHost == aHost; }

    private:
        UpdateMetadata(const Dns::UpdateHeader &aHeader, Host *aHost, const Ip6::MessageInfo &aMessageInfo);

        TimeMilli         mExpireTime;  // Expire time of this update; In milliseconds.
        Dns::UpdateHeader mDnsHeader;   // The header of the DNS update request.
        Host *            mHost;        // The host will be updated. The UpdateMetadata has no ownership of this host.
        Ip6::MessageInfo  mMessageInfo; // The message info of the DNS update request.
        UpdateMetadata *  mNext;        // The pointer to the next UpdateMetadata object.
    };

    void     Start(void);
    void     Stop(void);
    void     HandleNotifierEvents(Events aEvents);
    otError  PublishService(void);
    void     UnpublishService(void);
    uint32_t GrantLease(uint32_t aLease) const;
    uint32_t GrantKeyLease(uint32_t aKeyLease) const;
    void     HandleSrpUpdateResult(otError                  aError,
                                   const Dns::UpdateHeader &aDnsHeader,
                                   Host &                   aHost,
                                   const Ip6::MessageInfo & aMessageInfo);

    void    HandleDnsUpdate(Message &                aMessage,
                            const Ip6::MessageInfo & aMessageInfo,
                            const Dns::UpdateHeader &aDnsHeader,
                            uint16_t                 aOffset);
    otError ProcessZoneSection(const Message &          aMessage,
                               const Dns::UpdateHeader &aDnsHeader,
                               uint16_t &               aOffset,
                               Dns::Zone &              aZone);
    otError ProcessUpdateSection(Host &                   aHost,
                                 const Message &          aMessage,
                                 const Dns::UpdateHeader &aDnsHeader,
                                 const Dns::Zone &        aZone,
                                 uint16_t                 aHeaderOffset,
                                 uint16_t &               aOffset);
    otError ProcessAdditionalSection(Host *                   aHost,
                                     const Message &          aMessage,
                                     const Dns::UpdateHeader &aDnsHeader,
                                     uint16_t                 aHeaderOffset,
                                     uint16_t &               aOffset);
    otError VerifySignature(const Dns::Ecdsa256KeyRecord &aKey,
                            const Message &               aMessage,
                            Dns::UpdateHeader             aDnsHeader,
                            uint16_t                      aSigOffset,
                            uint16_t                      aSigRdataOffset,
                            uint16_t                      aSigRdataLength,
                            const char *                  aSignerName);

    static otError ProcessHostDescriptionInstruction(Host &                   aHost,
                                                     const Message &          aMessage,
                                                     const Dns::UpdateHeader &aDnsHeader,
                                                     const Dns::Zone &        aZone,
                                                     uint16_t                 aHeaderOffset,
                                                     uint16_t                 aOffset);
    static otError ProcessServiceDiscoveryInstructions(Host &                   aHost,
                                                       const Message &          aMessage,
                                                       const Dns::UpdateHeader &aDnsHeader,
                                                       const Dns::Zone &        aZone,
                                                       uint16_t                 aHeaderOffset,
                                                       uint16_t                 aOffset);
    static otError ProcessServiceDescriptionInstructions(Host &                   aHost,
                                                         const Message &          aMessage,
                                                         const Dns::UpdateHeader &aDnsHeader,
                                                         const Dns::Zone &        aZone,
                                                         uint16_t                 aHeaderOffset,
                                                         uint16_t &               aOffset);
    static bool    IsValidDeleteAllRecord(const Dns::ResourceRecord &aRecord);

    void HandleUpdate(const Dns::UpdateHeader &aDnsHeader, Host *aHost, const Ip6::MessageInfo &aMessageInfo);

    /**
     * This method adds a SRP service host and takes ownership of it.
     *
     * The caller MUST make sure that there is no existing host with the same hostname.
     *
     */
    void AddHost(Host *aHost);

    /**
     * This method removes and destroys the host.
     *
     */
    void        RemoveHost(Host *aHost);
    Service *   FindService(const char *aFullName);
    bool        HasNameConflictsWith(Host &aHost);
    void        SendResponse(const Dns::UpdateHeader &   aHeader,
                             Dns::UpdateHeader::Response aResponseCode,
                             const Ip6::MessageInfo &    aMessageInfo);
    void        SendResponse(const Dns::UpdateHeader &aHeader,
                             uint32_t                 aLease,
                             uint32_t                 aKeyLease,
                             const Ip6::MessageInfo & aMessageInfo);
    static void HandleUdpReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);
    void        HandleUdpReceive(Message &aMessage, const Ip6::MessageInfo &aMessageInfo);
    static void HandleLeaseTimer(Timer &aTimer);
    void        HandleLeaseTimer(void);
    static void HandleOutstandingUpdatesTimer(Timer &aTimer);
    void        HandleOutstandingUpdatesTimer(void);

    void HandleAdvertisingResult(UpdateMetadata *aUpdate, otError aError);

    bool mEnabled;

    Ip6::Udp::Socket              mSocket;
    otSrpServerAdvertisingHandler mAdvertisingHandler;
    void *                        mAdvertisingHandlerContext;

    uint32_t mMinLease;    // The minimum lease time in seconds.
    uint32_t mMaxLease;    // The maximum lease time in seconds.
    uint32_t mMinKeyLease; // The minimum key-lease time in seconds.
    uint32_t mMaxKeyLease; // The maximum key-lease time in seconds.

    LinkedList<Host> mHosts;
    TimerMilli       mLeaseTimer;

    TimerMilli                 mOutstandingUpdatesTimer;
    LinkedList<UpdateMetadata> mOutstandingUpdates;
};

} // namespace Srp

} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

#endif // BORDER_ROUTER_SRP_SERVER_HPP_
