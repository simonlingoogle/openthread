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
 *   This file includes the hardware abstraction layer for watchdog.
 *
 */

#include <board-config.h>

#include <assert.h>
#include <stddef.h>
#include <utils/code_utils.h>
#include <openthread/platform/toolchain.h>

#include <nrf_drv_clock.h>
#include <nrfx/mdk/nrf.h>

#include "chips/include/cmsis.h"
#include "chips/include/watchdog.h"

// The NRF52X Watchdog HW block is very flawed and will not provide very useful
// information when it trips so it's better to use one of the RTCs.
// Watchdog HW block flaws include:
//   - There is no way to pause/disable it once enabled.
//   - There is no way to change its configuration once enabled.
//   - A software reset doesn't reset the watchdog (so bootloader/early boot
//     code needs to be aware that the watchdog might already be running).
//   - The early warning interrupt mechanism is a regular interrupt and not a NMI.
//   - The early warning interrupt only allows for 2 clks at 32768Hz (about 60us)
//     before the watchdog reset occurs, which isn't enough time to do much.
//   - RAM is reset by the watchdog interrupt, so even if the early warning
//     interrupt could save all state to RAM, there's no guarantee that the RAM
//     contents will be valid on next boot.
//
// It's just better not to use the HW Watchdog block given its flaws. Instead,
// we'll use a RTC, which is even lower power, and has almost none of the flaws
// listed above. Two concerns when using a RTC timer for watchdog functionality:
//   - We can't configure the RTC timer as a NMI, so runaway code that leaves
//     interrupts disabled won't get caught by the RTC based watchdog functionality.
//     We can mitigate this by changing the tbHalInterruptDisable() function to
//     be like the FreeRTOS functions, where we change BASEPRI instead of PRIMASK.
//     We set RTC timer interrupt priority to 0 (highest priority configurable interrupt)
//     and outside of the range that is disabled by tbHalInterruptDisable(), and
//     make sure no one sets PRIMASK directly. Then the only code that can block the
//     RTC interrupt would be the HardFault handler, which has fixed priority of -1.
//   - We can't disable RTC timer automatically when a debugger is connected.
//     This could be mitigated by using openocd/gdb hooks to disable/enable the RTC
//     timer used whenever we halt/resume the processor, but that requires everyone
//     to have the right hooks setup. Instead we mitigate this case by using the
//     WDT->CYCCNT to count the number of processor clocks since the last time the
//     watchdog was refreshed. If the CYCCNT value isn't about the value expected
//     for the given timeout, then we assume this is because the debugger had stopped
//     the processor and so we self-refresh the RTC timer. We've tested that the
//     CYCCNT does count in WFE, so sleep with debugger connected does trip the
//     watchdog as expected.

// We cannot use tbHalInterruptDisable() to implement a critical section
// for this watchdog implementation since tbHalInterruptDisable() only
// disables interrupts at priorities lower than the priority being used for
// the RTC interrupt we're using. So instead, configure BASEPRI directly to
// block interrupt priorities at or below WATCHDOG_RTC_IRQ_PRIORITY if it's
// priority is non-zero. If WATCHDOG_RTC_IRQ_PRIORITY is 0, have to use
// PRIMASK because setting BASEPRI to 0 is equivalent to not masking.
#if WATCHDOG_RTC_IRQ_PRIORITY == 0
#define WATCHDOG_CRITICAL_SECTION_START()       \
    {                                           \
        uint32_t old_primask = __get_PRIMASK(); \
        __set_PRIMASK(1)

#define WATCHDOG_CRITICAL_SECTION_END() \
    __set_PRIMASK(old_primask);         \
    }
#else
#define WATCHDOG_CRITICAL_SECTION_START()        \
    {                                            \
        uint32_t old_base_pri = __get_BASEPRI(); \
        __set_BASEPRI(WATCHDOG_RTC_IRQ_PRIORITY << (8 - __NVIC_PRIO_BITS))

#define WATCHDOG_CRITICAL_SECTION_END() \
    __set_BASEPRI(old_base_pri);        \
    }
#endif // WATCHDOG_RTC_IRQ_PRIORITY == 0

// The timeout time of the watchdog timer, in seconds.
#ifndef WATCHDOG_TIMEOUT_IN_SECONDS
#define WATCHDOG_TIMEOUT_IN_SECONDS 12
#endif

// The minimum number of cycles we expected to have run since last time we
// refreshed the RTC timer. It should be related to the RTC timeout.
// At 64MHz we expect roughly 64000000 cycles per second, so
// WDT_MIN_CYCCNT_FOR_WATCHDOG_RESET should be
// 64000000 * WATCHDOG_TIMEOUT_IN_SECONDS.
//
// When tested, we found that less than 64000000 cycles were run in 12 seconds,
// maybe due to inaccuracy of the HCLK or some other reason. We'll use a more
// conservative number like 63000000, which appears to work.
//
// Note that since CYCCNT register is 32-bits, this limits the maximum value
// for WATCHDOG_TIMEOUT_IN_SECONDS that we can verify using the CYCCNT to
// about 68 seconds.
#define WDT_CYCCNT_MIN_CYCLES_PER_SECOND 63000000

#if (WATCHDOG_TIMEOUT_IN_SECONDS > (0xffffffff / WDT_CYCCNT_MIN_CYCLES_PER_SECOND))
#error "WATCHDOG_TIMEOUT_IN_SECONDS too large to check with CYCCNT"
#endif

#define WDT_MIN_CYCCNT_FOR_WATCHDOG_RESET (WDT_CYCCNT_MIN_CYCLES_PER_SECOND * WATCHDOG_TIMEOUT_IN_SECONDS)

#define WATCHDOG_RTC_CC_INDEX 1
#define WATCHDOG_RTC_INTENSET_COMPARE_MASK RTC_INTENSET_COMPARE1_Msk
#define WATCHDOG_RTC_INTENCLR_COMPARE_MASK RTC_INTENCLR_COMPARE1_Msk

static bool sDwtCycleCounterSupport = false;
static void watchdogEnable(void);

void tbWatchdogInit(void)
{
    // Check if a debugger is connected. If it is, attempt to use the DWT
    // cycle counter to track how many instructions excecuted since last
    // watchdog refresh. We expect it to be quite high except if debugger
    // is being used.
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)
    {
        // Check if the CYCCNT is supported. If it is, we'll use it
        // to sanity check CYCCNT when a prewatchdog_isr occurs.
        // If it's not available, we can't.
        if ((DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) == 0)
        {
            // Enable CYCCNT and set global that we should be checking
            // CYCCNT whenever the RTC interrupt fires.
            DWT->CYCCNT = 0;
            DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

            sDwtCycleCounterSupport = true;
        }
    }

    if (!nrf_drv_clock_lfclk_is_running())
    {
        // Setup low frequency clock.
        nrf_drv_clock_lfclk_request(NULL);

        while (!nrf_drv_clock_lfclk_is_running())
        {
        }
    }

    // Enable the watchdog.
    watchdogEnable();
}

static void watchdogEnable(void)
{
    NRF_RTC_Type *rtc = WATCHDOG_RTC;

    // Check whether the RTC timer has enabled.
    otEXPECT(rtc->INTENSET == 0);

    // Make sure RTC timer is stopped.
    rtc->TASKS_STOP = 1;

    // Clear the RTC timer counter.
    rtc->TASKS_CLEAR = 1;

    // Set timeout. Since resolution we need is typically just seconds,
    // use the largest PRESCALER value. PRESCALER register is 12-bits,
    // and setting it to 0xfff gives a COUNTER frequency of
    //      32768/(PRESCALER+1) = 8Hz, or 125ms.
    rtc->PRESCALER                 = RTC_PRESCALER_PRESCALER_Msk;
    rtc->CC[WATCHDOG_RTC_CC_INDEX] = WATCHDOG_TIMEOUT_IN_SECONDS * 8;

    // Enable RTC compare interrupt.
    rtc->INTENSET = WATCHDOG_RTC_INTENSET_COMPARE_MASK;

    // Enable watchdog RTC IRQ.
    NVIC_SetPriority(WATCHDOG_RTC_IRQN, WATCHDOG_RTC_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(WATCHDOG_RTC_IRQN);
    NVIC_EnableIRQ(WATCHDOG_RTC_IRQN);

    // Start the RTC timer.
    rtc->TASKS_START = 1;

    if (sDwtCycleCounterSupport)
    {
        // Clear DWT cycle counter.
        DWT->CYCCNT = 0;
    }

exit:
    return;
}

static void watchdogDisable(void)
{
    NRF_RTC_Type *rtc = WATCHDOG_RTC;

    // Disable the watch RTC IRQ.
    NVIC_DisableIRQ(WATCHDOG_RTC_IRQN);

    // Stop the RTC timer.
    rtc->INTENCLR   = WATCHDOG_RTC_INTENCLR_COMPARE_MASK;
    rtc->TASKS_STOP = 1;
}

static void watchdogRefresh(void)
{
    NRF_RTC_Type *rtc = WATCHDOG_RTC;

    // Clear RTC counter.
    rtc->TASKS_CLEAR = 1;

    // Clear DWT cycle counter if in use.
    if (sDwtCycleCounterSupport)
    {
        DWT->CYCCNT = 0;
    }
}

void tbHalWatchdogSetEnabled(bool aEnabled)
{
    WATCHDOG_CRITICAL_SECTION_START();

    if (aEnabled)
    {
        watchdogEnable();
    }
    else
    {
        watchdogDisable();
    }

    WATCHDOG_CRITICAL_SECTION_END();
}

bool tbHalWatchdogIsEnabled(void)
{
    return (WATCHDOG_RTC->INTENSET != 0);
}

void tbHalWatchdogRefresh(void)
{
    watchdogRefresh();
}

static bool watchdogIgnoreInterrupt(void)
{
    bool ignoreWatchdogInterrupt = false;

    // If we have a cycle counter, check if it is sufficiently high that
    // we believe the debugger wasn't being actively used and should go
    // ahead and trigger the watchdog. Otherwise, refresh the RTC (but
    // not cycle counter) and try again next time the RTC interrupt occurs.
    //
    // If no cycle counter, but we detect a connected debugger, we'll
    // just ignore watchdogs (watchdog testing will require disconnecting
    // the debugger).
    if (((sDwtCycleCounterSupport == true) && (DWT->CYCCNT < WDT_MIN_CYCCNT_FOR_WATCHDOG_RESET)) ||
        ((sDwtCycleCounterSupport == false) && (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)))
    {
        // Ignore RTC timeout because either too few cycles since last refresh
        // or no cycle counter but we detect debugger is connected.
        // Reset RTC and try again.
        NRF_RTC_Type *rtc = WATCHDOG_RTC;

        ignoreWatchdogInterrupt = true;

        // Clear event so it will trigger again.
        rtc->EVENTS_COMPARE[WATCHDOG_RTC_CC_INDEX] = 0;

        NVIC_ClearPendingIRQ(WATCHDOG_RTC_IRQN);
        watchdogRefresh();
    }

    return ignoreWatchdogInterrupt;
}

void WATCHDOG_RTC_IRQ_HANDLER(void)
{
    if (!watchdogIgnoreInterrupt())
    {
        NVIC_SystemReset();
    }
}
