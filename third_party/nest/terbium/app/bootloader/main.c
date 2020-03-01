/*
 *  Copyright (c) 2020 Google LLC
 *  All rights reserved.
 *
 *  This document is the property of Google LLC, Inc. It is
 *  considered proprietary and confidential information.
 *
 *  This document may not be reproduced or transmitted in any form,
 *  in whole or in part, without the express written permission of
 *  Google LLC.
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/boot.h"

static void __attribute__((noreturn, naked)) bootloaderJumpToImage(const uint32_t aResetVector, const uint32_t aSp)
{
    (void)(aResetVector);
    (void)(aSp);

    __asm volatile("mov    sp, r1        \n\t"
                   "bx     r0            \n\t");
}

static uint32_t getAppResetVector(void)
{
    return (uint32_t)(((tbAppImageHeader *)TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS)->mAppEntryPoint);
}

static uint32_t getAppSp(void)
{
    return (uint32_t)(((tbAppImageHeader *)TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS)->mSpMain);
}

int main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);

    bootloaderJumpToImage(getAppResetVector(), getAppSp());

    while (1)
    {
    }
}
