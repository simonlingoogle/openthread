/*
 *  Copyright (c) 2020 Google LLC.
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

/**
 * @file
 *  This file implements bootloader boots up related variables and functions.
 */

#include "platform/boot.h"

#ifdef __cplusplus
extern "C" {
#endif

// These variables should be defined in the linker script or start up files.
extern uint32_t const __VECTOR__table;
extern uint32_t const __STACK__end;
extern uint32_t const __SIGNATURE__start;
extern void           Reset_Handler(void);

#ifdef __cplusplus
} // extern "C"
#endif

// This will declare a constant unsigned char array in Flash. This array is then
// placed in the .signature section in the resulting linked ELF file. A script
// will fill .signature section with ECDSA signature after the ELF file is generated.
const uint8_t sSignature[TERBIUM_MEMORY_SIGNATURE_SIZE] __attribute__((used))
__attribute__((section(".signature"))) = {0};

// Marble bootloader stores boot counters in .preserve section.
// This will declare a structue in RAM to prevent .preserve section from being
// accidentally changed by the app.
const tbBootMetrics gBootMetrics __attribute__((used)) __attribute__((section(".preserve"))) = {};

// Marble bootloader uses the .guard section to guard the CSTACK. When Marble
// bootloader boots up, it calls the MPU to set .guard section in RAM to
// read-only. The .guard section is under CSTACK. If CSTACK is overflow,
// CPU will write data to .guard section, which trigers an exception.
// This will declare a constant unsigned char array in RAM to prevent app from
// writing data to the .guard section.
const uint8_t sGuard[TERBIUM_MEMORY_GUARD_RAM_SIZE] __attribute__((used)) __attribute__((section(".guard"))) = {0};

// The .aat section is put at the beginning of an app image. Bootloader reads
// .aat section to get app info to verify the signature and then jump to the
// app image.
const tbAppImageHeader sAppImageHeader __attribute__((used))
__attribute((section(".aat"))) = {(uint32_t)&__STACK__end, Reset_Handler, (void *)&__SIGNATURE__start};

uint32_t tbBootGetVectorTableAddress(void)
{
    return (uint32_t)&__VECTOR__table;
}
