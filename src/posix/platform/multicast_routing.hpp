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

#ifndef POSIX_PLATFORM_MULTICAST_ROUTING_HPP_
#define POSIX_PLATFORM_MULTICAST_ROUTING_HPP_

#if OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#include "openthread-posix-config.h"
#include "platform-posix.h"

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <map>
#include <set>
#include <openthread/backbone_router_ftd.h>
#include <openthread/openthread-system.h>

#include "core/common/non_copyable.hpp"
#include "core/net/ip6_address.hpp"
#include "lib/url/url.hpp"

namespace ot {
namespace Posix {

/**
 * This class implements Multicast Routing management.
 *
 */
class MulticastRoutingManager : private NonCopyable
{
public:
    /**
     * This constructor initializes a Multicast Routing manager instance.
     *
     */
    explicit MulticastRoutingManager()
        : mMulticastRouterSock(-1)
    {
    }

    /**
     * This method initializes the Multicast Routing manager.
     *
     * @param[in]  aInstance  A pointer to an OpenThread instance.
     *
     */
    void Init(otInstance *aInstance);

    /**
     * This method updates the fd_set and timeout for mainloop.
     *
     * @param[inout]    aReadFdSet      A reference to fd_set for polling read.
     * @param[inout]    aWriteFdSet     A reference to fd_set for polling read.
     * @param[inout]    aErrorFdSet     A reference to fd_set for polling error.
     * @param[inout]    aMaxFd          A reference to the current max fd in @p aReadFdSet and @p aWriteFdSet.
     * @param[inout]    aTimeout        A reference to the timeout.
     *
     */
    void UpdateFdSet(fd_set &aReadFdSet, int &aMaxFd) const;

    /**
     * This method performs Multicast Routing processing.
     *
     * @param[in]   aReadFdSet   A reference to read file descriptors.
     * @param[in]   aWriteFdSet  A reference to write file descriptors.
     * @param[in]   aErrorFdSet  A reference to error file descriptors.
     *
     */
    void Process(const fd_set &aReadFdSet);

    /**
     * This method handles Thread state changes.
     *
     * @param[in] aInstance  A pointer to an OpenThread instance.
     * @param[in] aFlags     Flags that denote the state change events.
     *
     */
    void HandleStateChange(otInstance *aInstance, otChangedFlags aFlags);

private:
    enum
    {
        kMulticastForwardingCacheExpireTimeout = 300, //< Expire timeout of Multicast Forwarding Cache (in seconds)
    };

    enum MifIndex : uint8_t
    {
        kMifIndexNone     = 0xff,
        kMifIndexThread   = 0,
        kMifIndexBackbone = 1,
    };

    class MulticastRoute
    {
        friend class MulticastRoutingManager;

    public:
        MulticastRoute(const Ip6::Address &aSrcAddr, const Ip6::Address &aGroupAddr)
            : mSrcAddr(aSrcAddr)
            , mGroupAddr(aGroupAddr)
        {
        }

        bool operator<(const MulticastRoute &aOther) const;

    private:
        Ip6::Address mSrcAddr;
        Ip6::Address mGroupAddr;
    };

    typedef std::chrono::steady_clock Clock;

    class MulticastRouteInfo
    {
        friend class MulticastRoutingManager;

    public:
        MulticastRouteInfo(MifIndex aIif, MifIndex aOif)
            : mValidPktCnt(0)
            , mOif(aOif)
            , mIif(aIif)
        {
            mLastUseTime = Clock::now();
        }

        MulticastRouteInfo() = default;

    private:
        std::chrono::time_point<Clock> mLastUseTime;
        unsigned long                  mValidPktCnt;
        MifIndex                       mOif;
        MifIndex                       mIif;
    };

    void    Enable(void);
    void    Disable(void);
    void    Add(const Ip6::Address &aAddress);
    void    Remove(const Ip6::Address &aAddress);
    bool    IsEnabled(void) const { return mMulticastRouterSock >= 0; }
    otError InitMulticastRouterSock(void);
    void    FinalizeMulticastRouterSock(void);
    void    ProcessMulticastRouterMessages(void);
    otError AddMulticastForwardingCache(const Ip6::Address &aSrcAddr, const Ip6::Address &aGroupAddr, MifIndex aIif);
    void    RemoveInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr);
    void    UnblockInboundMulticastForwardingCache(const Ip6::Address &aGroupAddr);
    void    ExpireMulticastForwardingCache(void);
    bool    UpdateMulticastRouteInfo(const MulticastRoute &route);
    static const char *MifIndexToString(MifIndex aMif);
    void               DumpMulticastForwardingCache(void);
    static void        HandleBackboneMulticastListenerEvent(void *                                 aContext,
                                                            otBackboneRouterMulticastListenerEvent aEvent,
                                                            const otIp6Address *                   aAddress);
    void               HandleBackboneMulticastListenerEvent(otBackboneRouterMulticastListenerEvent aEvent,
                                                            const Ip6::Address &                   aAddress);

    std::set<Ip6::Address>                       mListenerSet;
    std::map<MulticastRoute, MulticastRouteInfo> mMulticastForwardingCache;
    int                                          mMulticastRouterSock;
};

} // namespace Posix
} // namespace ot

#endif // OPENTHREAD_CONFIG_BACKBONE_ROUTER_ENABLE

#endif // POSIX_PLATFORM_MULTICAST_ROUTING_HPP_
