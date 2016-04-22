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
 *   This file contains definitions for computing HMACs.
 */

#ifndef HMAC_HPP_
#define HMAC_HPP_

#include <stdint.h>

#include <common/thread_error.hpp>
#include <crypto/hash.hpp>

namespace Thread {
namespace Crypto {

class Hmac
{
public:
    explicit Hmac(Hash &hash);
    ThreadError SetKey(const void *key, uint16_t keyLength);
    ThreadError Init();
    ThreadError Input(const void *buf, uint16_t bufLength);
    ThreadError Finalize(uint8_t *hash);

private:
    enum
    {
        kMaxKeyLength = 64,
    };
    uint8_t mKey[kMaxKeyLength];
    uint8_t mKeyLength = 0;
    Hash *mHash;
};

}  // namespace Crypto
}  // namespace Thread

#endif  // CRYPTO_HMAC_HPP_
