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
 *   This file implements the delay functions.
 *
 */

#include <assert.h>

#include "nrf_delay.h"
#include "chips/include/delay.h"

void tbHalDelayUs(uint32_t aDelayUs)
{
    assert(aDelayUs <= TB_HAL_DELAY_US_MAX);
    nrf_delay_us(aDelayUs);
}

void tbHalDelayMs(uint32_t aDelayMs)
{
    assert(aDelayMs <= TB_HAL_DELAY_MS_MAX);
    nrf_delay_us(aDelayMs * TH_HAL_USEC_PER_MSEC);
}
