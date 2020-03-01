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
/*
 * @file
 *   This file includes the definition of blocking delay functions.
 *
 */

#ifndef CHIPS_INCLUDE_DELAY_H__
#define CHIPS_INCLUDE_DELAY_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TH_HAL_USEC_PER_MSEC 1000                                        ///< Microseconds per millisecond.
#define TB_HAL_DELAY_US_MAX (0xffffffff >> 1)                            ///< Maximum delay microseconds.
#define TB_HAL_DELAY_MS_MAX (TB_HAL_DELAY_US_MAX / TH_HAL_USEC_PER_MSEC) ///< Maximum delay milliseconds.

/**
 * This method blocks for specified microseconds.
 *
 * @param[in] aDelayUs  The time delay in microseconds.
 *
 */
void tbHalDelayUs(uint32_t aDelayUs);

/**
 * This method blocks for specified microseconds.
 *
 * @param[in] aDelayMs  The time delay in milliseconds.
 *
 */
void tbHalDelayMs(uint32_t aDelayMs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_DELAY_H__
