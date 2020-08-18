/*
 *  Copyright (c) 2018, The OpenThread Authors.
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

#include "test_platform.h"

#include <chrono>
#include <iostream>
#include <map>

#include <unordered_map>
#include <openthread/config.h>

#include "test_util.h"
#include "common/code_utils.hpp"
#include "common/instance.hpp"
#include "thread/child_table.hpp"

namespace std {

template <> struct hash<ot::Ip6::Address>
{
    std::size_t operator()(const ot::Ip6::Address &k) const
    {
        size_t ret = 0x4234234;

        for (uint32_t b : k.mFields.m32)
        {
            ret ^= b;
        }
        return ret;
    }
};

} // namespace std

namespace ot {

static ot::Instance *sInstance;

constexpr int N = 250;

class NdProxyTableArray : public Clearable<NdProxyTableArray>
{
public:
    NdProxyTableArray() { Clear(); }

    void Add(const Ip6::Address &aAddress)
    {
        NdProxyEntry *invalid = nullptr;

        for (auto &entry : mAddresses)
        {
            if (entry.mValid)
            {
                if (entry.mAddress == aAddress)
                {
                    ExitNow();
                }
            }
            else
            {
                if (invalid == nullptr)
                {
                    invalid = &entry;
                }
            }
        }

        assert(invalid != nullptr);

        invalid->mAddress = aAddress;
        invalid->mValid   = true;

    exit:
        return;
    }

    bool Find(const Ip6::Address &aAddress)
    {
        bool ret = false;

        for (auto &entry : mAddresses)
        {
            if (entry.mValid && entry.mAddress == aAddress)
            {
                ExitNow(ret = true);
            }
        }
    exit:
        return ret;
    }

private:
    typedef struct NdProxyEntry
    {
        Ip6::Address mAddress;
        bool         mValid;
    } NdProxyEntry;

    NdProxyEntry mAddresses[N];
};

class NdProxyTableHashmap
{
public:
    NdProxyTableHashmap() {}

    void Add(const Ip6::Address &aAddress) { mAddresses[aAddress] = true; }
    bool Find(const Ip6::Address &aAddress) { return mAddresses.find(aAddress) != mAddresses.end(); }

private:
    std::unordered_map<Ip6::Address, bool> mAddresses;
};

template <typename T> void TestNdProxyTable(T &table)
{
    Ip6::Address lastAddress;

    for (int i = 0; i < N; i++)
    {
        uint8_t      bytes[] = {0x20,
                           0x01,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           0x00,
                           static_cast<uint8_t>(i),
                           static_cast<uint8_t>(i >> 8),
                           static_cast<uint8_t>(i >> 16),
                           static_cast<uint8_t>(i >> 24),
                           0x00,
                           0x00,
                           0x00,
                           0x00};
        Ip6::Address address;

        memcpy(address.mFields.m8, bytes, 16);
//        address.mFields.m8[0] = i;

        table.Add(address);
        lastAddress = address;
    }

    auto t0 = std::chrono::system_clock::now();

    for (int i = 0; i < 1000; i++)
    {
        table.Find(lastAddress);
    }

    auto t1 = std::chrono::system_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count() << std::endl;
}

} // namespace ot

int main(void)
{
    ot::NdProxyTableArray   array;
    ot::NdProxyTableHashmap hashmap;

    ot::TestNdProxyTable<ot::NdProxyTableArray>(array);
    ot::TestNdProxyTable<ot::NdProxyTableHashmap>(hashmap);
    printf("\nAll tests passed.\n");
    return 0;
}
