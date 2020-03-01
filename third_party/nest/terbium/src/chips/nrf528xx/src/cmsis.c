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
 *   This file implements the hardware abstraction layer for cmsis.
 *
 */

#include <board-config.h>

#include <assert.h>
#include <stdint.h>

#include <nrfx/mdk/nrf.h>

#include <cmsis/cmsis_gcc.h>
#include <cmsis/core_cm4.h>

static volatile uint8_t sIntCount;

void tbHalInterruptDisable(void)
{
#ifdef INTERRUPT_DISABLE_BASE_PRIORITY_VALUE
    __set_BASEPRI(INTERRUPT_DISABLE_BASE_PRIORITY_VALUE);
#else
    __disable_irq();
#endif

    sIntCount++;
}

void tbHalInterruptEnable(void)
{
    assert(sIntCount > 0);

    if ((--sIntCount) == 0)
    {
#ifdef INTERRUPT_DISABLE_BASE_PRIORITY_VALUE
        __set_BASEPRI(0);
#else
        __enable_irq();
#endif
    }
}
