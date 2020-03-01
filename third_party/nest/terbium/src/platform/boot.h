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

#ifndef PLATFORM_BOOT_H__
#define PLATFORM_BOOT_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure represents the boot information of app image. It is placed at
 * the beginning of an app image. Bootloader reads this structure to get
 * information about app image.
 *
 */
typedef struct
{
    uint32_t mSpMain;             ///< Pointer to stack top.
    void (*mAppEntryPoint)(void); ///< Pointer to entry address of the app image.
    void *mSignatureAddress;      ///< Pointer to the address of image signature.
                                  ///< It is also point to the end of the image
                                  ///< since the signature is placed by linker
                                  ///< script at the end.
} tbAppImageHeader;

/**
 * This preserved/persistent RAM data structure is used to track boot counts
 * for identifying boot loops.
 *
 */
typedef struct
{
    uint64_t mTotalBootCount; ///< The total number of boot loops. It is
                              ///< incremented by bootloader on every boot.
    uint32_t mBootLoopCount;  ///< The number of boot loops. It is incremented
                              ///< by bootloader on every boot. Reset it to 0
                              ///< whenever bootloader enter boot cmd mode.
} tbBootMetrics;

/**
 * This function returns the app vector table address.
 *
 * @returns The vector table address.
 *
 */
uint32_t tbBootGetVectorTableAddress(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_BOOT_H__
