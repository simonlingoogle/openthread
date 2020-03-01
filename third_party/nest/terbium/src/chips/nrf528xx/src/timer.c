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
 *   This file implements the hardware abstraction layer for timer.
 *
 */

#include <board-config.h>

#include <assert.h>
#include <stddef.h>

#include <nrfx/mdk/nrf.h>
#include <utils/code_utils.h>
#include <openthread/platform/toolchain.h>

#include "chips/include/cmsis.h"
#include "chips/include/timer.h"

/**
 *  NOTE:
 *    (1) TIMER 0 is used by the radio driver for precise frame timestamps and
 *        synchronous radio operations. TIMER 1 is used both by the radio driver
 *        for ACK IFS and by the FEM module.
 *        This timer driver do not allow to configure TIMER0 and TIMER1.
 *
 *    (2) SHUTDOWN register is marked "deprecated" but actually has usefulness.
 *        Power can be reduced if the timer is SHUTDOWN. If the timer has been
 *        SHUTDOWN, capture will return 0.
 *
 *    (3) Each TIMER has a number of Compare/Capture (CC) registers. HW instance
 *        TIMER0-TIMER2 have 4, TIMER3 and TIMER4 have 6.
 *        This driver uses CC[0] for compare time for one-shot start time.
 *        This driver uses CC[1] for capturing the current value of the internal COUNTER.
 */

#define TIMER_NUM_RESERVED 2                               ///< Number of reserved timers.
#define TIMER_NUM_VALID (TIMER_COUNT - TIMER_NUM_RESERVED) ///< Number of valid timers.
#define TIMER_INDEX(aTimer) ((aTimer)-TIMER_NUM_RESERVED)  ///< Index of the timer.

#if defined(NRF52840_XXAA)
#include <nrfx/mdk/nrf52840_peripherals.h>
static const IRQn_Type sTimerIrqs[TIMER_NUM_VALID] = {TIMER2_IRQn, TIMER3_IRQn, TIMER4_IRQn};
static NRF_TIMER_Type *sTimers[TIMER_NUM_VALID]    = {NRF_TIMER2, NRF_TIMER3, NRF_TIMER4};
#elif defined(NRF52811_XXAA)
#include <nrfx/mdk/nrf52811_peripherals.h>
static const IRQn_Type sTimerIrqs[TIMER_NUM_VALID] = {TIMER2_IRQn};
static NRF_TIMER_Type *sTimers[TIMER_NUM_VALID]    = {NRF_TIMER2};
#endif

typedef struct TimerConfig
{
    tbHalTimerIntCallback mCallback;
    void *                mContext;
} TimerConfig;

static TimerConfig       sTimerConfigs[TIMER_NUM_VALID];
static volatile uint32_t sElapsedUs[TIMER_NUM_VALID];
static volatile bool     sInitilaized[TIMER_NUM_VALID];

static inline NRF_TIMER_Type *timerGet(tbHalTimer aTimer)
{
    assert((aTimer != 0) && (aTimer != 1) && (aTimer < TIMER_COUNT));
    return sTimers[TIMER_INDEX(aTimer)];
}

static uint32_t timerGetNow(tbHalTimer aTimer)
{
    NRF_TIMER_Type *timer = timerGet(aTimer);

    // Capture timer value to CC[1] register
    timer->TASKS_CAPTURE[1] = 1;
    return timer->CC[1];
}

static bool timerIsRunning(tbHalTimer aTimer)
{
    NRF_TIMER_Type *timer = timerGet(aTimer);

    return (timer->INTENSET & TIMER_INTENSET_COMPARE0_Msk) == TIMER_INTENSET_COMPARE0_Msk;
}

otError tbHalTimerInit(tbHalTimer aTimer, tbHalTimerIntCallback aCallback, void *aContext)
{
    otError         error = OT_ERROR_NONE;
    NRF_TIMER_Type *timer = timerGet(aTimer);
    uint8_t         index = TIMER_INDEX(aTimer);

    otEXPECT_ACTION(!sInitilaized[index], error = OT_ERROR_FAILED);
    sInitilaized[index] = true;

    sTimerConfigs[index].mCallback = aCallback;
    sTimerConfigs[index].mContext  = aContext;

    // Disable interrupt for COMPARE[0] event, in case COMPARE[0] event has been enabled.
    timer->INTENCLR = TIMER_INTENSET_COMPARE0_Msk;

    // Set timer IRQ priority and enable interrupt.
    NVIC_SetPriority(sTimerIrqs[index], TIMER_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(sTimerIrqs[index]);
    NVIC_EnableIRQ(sTimerIrqs[index]);

    timer->TASKS_STOP     = 1; // Stop timer.
    timer->TASKS_SHUTDOWN = 1; // Shutdown to save power. Shutting down also clears COUNTER.

exit:
    return error;
}

void tbHalTimerDeinit(tbHalTimer aTimer)
{
    NRF_TIMER_Type *timer = timerGet(aTimer);
    uint8_t         index = TIMER_INDEX(aTimer);

    otEXPECT(sInitilaized[index]);

    timer->TASKS_STOP        = 1;                           // Stop timer.
    timer->TASKS_SHUTDOWN    = 1;                           // Shutdown to save power.
    timer->INTENCLR          = TIMER_INTENSET_COMPARE0_Msk; // Disable interrupt for COMPARE[0] event.
    timer->EVENTS_COMPARE[0] = 0;

    sInitilaized[index] = false;
    NVIC_DisableIRQ(sTimerIrqs[index]);

exit:
    return;
}

otError tbHalTimerStart(tbHalTimer aTimer, uint32_t aTime)
{
    otError         error = OT_ERROR_NONE;
    NRF_TIMER_Type *timer = timerGet(aTimer);
    uint8_t         index = TIMER_INDEX(aTimer);

    otEXPECT_ACTION(sInitilaized[index], error = OT_ERROR_FAILED);

    tbHalInterruptDisable();

    timer->TASKS_STOP  = 1;     // Stop timer in case it has been started before.
    timer->TASKS_CLEAR = 1;     // Clear counter.
    timer->CC[0]       = aTime; // Set counter compare value.
    sElapsedUs[index]  = 0;     // Clear elapsed time.

    // Default PRESCALER of 1MHz is what we want so we don't change it.
    timer->BITMODE           = TIMER_BITMODE_BITMODE_32Bit; // 32-bit timer.
    timer->MODE              = TIMER_MODE_MODE_Timer;       // Timer mode.
    timer->EVENTS_COMPARE[0] = 0;                           // Clear COMPARE[0] EVENT flag.
    timer->INTENSET          = TIMER_INTENSET_COMPARE0_Msk; // Enable interrupt for COMPARE[0] EVENT.

    // Configure SHORTS to STOP when the COMPARE[0] EVENT occurs.
    timer->SHORTS = (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);

    // Start timer.
    timer->TASKS_START = 1;

    tbHalInterruptEnable();

exit:
    return error;
}

otError tbHalTimerStop(tbHalTimer aTimer)
{
    otError         error = OT_ERROR_NONE;
    NRF_TIMER_Type *timer = timerGet(aTimer);
    uint8_t         index = TIMER_INDEX(aTimer);

    otEXPECT_ACTION(sInitilaized[index], error = OT_ERROR_FAILED);

    tbHalInterruptDisable();

    sElapsedUs[index] = timerGetNow(aTimer);

    // Disable COMPARE[0] EVENT.
    timer->INTENCLR = TIMER_INTENCLR_COMPARE0_Msk;

    // Shutdown timer to save power.
    timer->TASKS_STOP     = 1;
    timer->TASKS_SHUTDOWN = 1;

    tbHalInterruptEnable();

exit:
    return error;
}

bool tbHalTimerIsRunning(tbHalTimer aTimer)
{
    return timerIsRunning(aTimer);
}

uint32_t tbHalTimerGetElapsedTime(tbHalTimer aTimer)
{
    return (timerIsRunning(aTimer)) ? timerGetNow(aTimer) : sElapsedUs[TIMER_INDEX(aTimer)];
}

static void timerIrqHandler(tbHalTimer aTimer)
{
    NRF_TIMER_Type *timer = timerGet(aTimer);
    uint8_t         index = TIMER_INDEX(aTimer);
    uint32_t        now   = timerGetNow(aTimer);

    otEXPECT((timer->INTENSET & TIMER_INTENSET_COMPARE0_Msk) && timer->EVENTS_COMPARE[0]);

    // Disable interrupt and clear the COMPARE[0] event.
    timer->INTENCLR          = TIMER_INTENSET_COMPARE0_Msk;
    timer->EVENTS_COMPARE[0] = 0;

    // Shutdown timer to save power.
    timer->TASKS_SHUTDOWN = 1;

    if (sTimerConfigs[index].mCallback != NULL)
    {
        sElapsedUs[index] = now;
        sTimerConfigs[index].mCallback(aTimer, sTimerConfigs[index].mContext);
    }

exit:
    return;
}

void TIMER2_IRQHandler(void)
{
    timerIrqHandler(2);
}

#if defined(NRF52840_XXAA)
void TIMER3_IRQHandler(void)
{
    timerIrqHandler(3);
}

void TIMER4_IRQHandler(void)
{
    timerIrqHandler(4);
}
#endif
