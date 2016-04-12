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

#ifndef MAC_WHITELIST_H_
#define MAC_WHITELIST_H_

#include <stdint.h>
#include <common/thread_error.h>
#include <mac/mac_frame.h>

namespace Thread {
namespace Mac {

class Whitelist
{
public:
    Whitelist();

    ThreadError Enable();
    ThreadError Disable();
    bool IsEnabled() const;

    int GetMaxEntries() const;

    int Add(const Address64 &address);
    ThreadError Remove(const Address64 &address);
    ThreadError Clear();

    int Find(const Address64 &address) const;

    const uint8_t *GetAddress(uint8_t entry) const;

    ThreadError ClearRssi(uint8_t entry);
    ThreadError GetRssi(uint8_t entry, int8_t &rssi) const;
    ThreadError SetRssi(uint8_t entry, int8_t rssi);

private:
    enum
    {
        kMaxEntries = 32,
    };

    struct Entry
    {
        Address64 addr64;
        int8_t rssi;
        bool valid : 1;
        bool rssi_valid : 1;
    };
    Entry m_whitelist[kMaxEntries];

    bool m_enabled = false;
};

}  // namespace Mac
}  // namespace Thread

#endif  // MAC_MAC_WHITELIST_H_
