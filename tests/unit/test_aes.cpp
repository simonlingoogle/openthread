/*
 *
 *    Copyright (c) 2015 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    @author  Jonathan Hui <jonhui@nestlabs.com>
 *
 */

#include "test_util.h"
#include <common/debug.hpp>
#include <crypto/aes_ccm.hpp>
#include <crypto/aes_ecb.hpp>
#include <string.h>

/**
 * Verifies test vectors from IEEE 802.15.4-2006 Annex C Section C.2.1
 */
void TestMacBeaconFrame()
{
    uint8_t key[] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    uint8_t test[] =
    {
        0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x02, 0x05, 0x00,
        0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
        0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A,
        0xB5, 0x53
    };

    uint8_t encrypted[] =
    {
        0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x02, 0x05, 0x00,
        0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
        0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A,
        0xB5, 0x53
    };

    uint8_t decrypted[] =
    {
        0x08, 0xD0, 0x84, 0x21, 0x43, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x02, 0x05, 0x00,
        0x00, 0x00, 0x55, 0xCF, 0x00, 0x00, 0x51, 0x52,
        0x53, 0x54, 0x22, 0x3B, 0xC1, 0xEC, 0x84, 0x1A,
        0xB5, 0x53
    };

    Thread::Crypto::AesEcb aes_ecb;
    aes_ecb.SetKey(key, sizeof(key));

    Thread::Crypto::AesCcm aes_ccm;
    uint32_t header_length = sizeof(test) - 8;
    uint32_t payload_length = 0;
    uint8_t tag_length = 8;

    uint8_t nonce[] =
    {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x05, 0x02,
    };

    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Finalize(test + header_length, &tag_length);

    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0,
                 "TestMacBeaconFrame encrypt failed");

    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Finalize(test + header_length, &tag_length);

    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0,
                 "TestMacBeaconFrame decrypt failed");
}

/**
 * Verifies test vectors from IEEE 802.15.4-2006 Annex C Section C.2.1
 */
void TestMacDataFrame()
{
    uint8_t key[] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    uint8_t test[] =
    {
        0x69, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x04, 0x05, 0x00,
        0x00, 0x00, 0x61, 0x62, 0x63, 0x64
    };

    uint8_t encrypted[] =
    {
        0x69, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x04, 0x05, 0x00,
        0x00, 0x00, 0xD4, 0x3E, 0x02, 0x2B
    };

    uint8_t decrypted[] =
    {
        0x69, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0x04, 0x05, 0x00,
        0x00, 0x00, 0x61, 0x62, 0x63, 0x64
    };

    Thread::Crypto::AesEcb aes_ecb;
    aes_ecb.SetKey(key, sizeof(key));

    Thread::Crypto::AesCcm aes_ccm;
    uint32_t header_length = sizeof(test) - 4;
    uint32_t payload_length = 4;
    uint8_t tag_length = 0;

    uint8_t nonce[] =
    {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x05, 0x04,
    };

    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Payload(test + header_length, test + header_length, payload_length, true);
    aes_ccm.Finalize(test + header_length + payload_length, &tag_length);

    dump("test", test, sizeof(test));
    dump("encrypted", encrypted, sizeof(encrypted));

    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0,
                 "TestMacDataFrame encrypt failed");

    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Payload(test + header_length, test + header_length, payload_length, false);
    aes_ccm.Finalize(test + header_length + payload_length, &tag_length);

    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0,
                 "TestMacDataFrame decrypt failed");
}

/**
 * Verifies test vectors from IEEE 802.15.4-2006 Annex C Section C.2.3
 */
void TestMacCommandFrame()
{
    uint8_t key[] =
    {
        0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    };

    uint8_t test[] =
    {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06,
        0x05, 0x00, 0x00, 0x00, 0x01, 0xCE, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    uint32_t header_length = 29, payload_length = 1;
    uint8_t tag_length = 8;

    uint8_t encrypted[] =
    {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06,
        0x05, 0x00, 0x00, 0x00, 0x01, 0xD8, 0x4F, 0xDE,
        0x52, 0x90, 0x61, 0xF9, 0xC6, 0xF1,
    };

    uint8_t decrypted[] =
    {
        0x2B, 0xDC, 0x84, 0x21, 0x43, 0x02, 0x00, 0x00,
        0x00, 0x00, 0x48, 0xDE, 0xAC, 0xFF, 0xFF, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC, 0x06,
        0x05, 0x00, 0x00, 0x00, 0x01, 0xCE, 0x4F, 0xDE,
        0x52, 0x90, 0x61, 0xF9, 0xC6, 0xF1,
    };

    uint8_t nonce[] =
    {
        0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x05, 0x06,
    };

    Thread::Crypto::AesEcb aes_ecb;
    aes_ecb.SetKey(key, sizeof(key));

    Thread::Crypto::AesCcm aes_ccm;
    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Payload(test + header_length, test + header_length, payload_length, true);
    aes_ccm.Finalize(test + header_length + payload_length, &tag_length);
    VerifyOrQuit(memcmp(test, encrypted, sizeof(encrypted)) == 0,
                 "TestMacCommandFrame encrypt failed\n");

    aes_ccm.Init(aes_ecb, header_length, payload_length, tag_length, nonce, sizeof(nonce));
    aes_ccm.Header(test, header_length);
    aes_ccm.Payload(test + header_length, test + header_length, payload_length, false);
    aes_ccm.Finalize(test + header_length + payload_length, &tag_length);

    VerifyOrQuit(memcmp(test, decrypted, sizeof(decrypted)) == 0,
                 "TestMacCommandFrame decrypt failed\n");
}

int main()
{
    TestMacBeaconFrame();
    TestMacDataFrame();
    TestMacCommandFrame();
    printf("All tests passed\n");
    return 0;
}
