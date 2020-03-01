/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file includes the platform-specific initializers.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <terbium-config.h>

#include <openthread/platform/logging.h>

#include "openthread-system.h"
#include "platform-nrf5-transport.h"
#include "platform-nrf5.h"
#include <drivers/clock/nrf_drv_clock.h>
#include <nrf.h>

#include <openthread/config.h>

#if !OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS && PLATFORM_OPENTHREAD_VANILLA

#include <mbedtls/platform.h>
#include <mbedtls/threading.h>

#include <openthread/heap.h>

#endif

#include "chips/include/gpio.h"
#include "chips/include/watchdog.h"
#include "platform/boot.h"
#include "platform/power_settings.h"
#include "platform/wireless_cal.h"
#if ((OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE == 1) && (TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE == 1))
#include "platform/coex.h"
#endif

extern bool gPlatformPseudoResetWasRequested;

static void nrf5MarbleWatchdogDisable(void)
{
    // Marble uses a RTC timer for watchdog functionality rather than the
    // WDT HW block because on nRF528xx, the WDT HW block is very limited
    // (can't disable it, doesn't give a useful early warning NMI, etc).
    // Marble bootloader has enabled Watchdog, so this function disables
    // the Marble watchdog.
    // To save power in Marble, a RTC timer is shared between tick and watchdog.
    // The COMPARE0 is used for tick and sleep wakeup interrupt. COMPARE1 is
    // used for watchdog, with COMPARE[1] event generating a EGU interrupt
    // using PPI at a higher priority. The following flow chart shows the
    // interrupt chain of watchdog in Marble:
    //
    //                         PPI_CH_19
    // rtc->EVENTS_COMPARE[1] -----------> NRF_EGU1->TASKS_TRIGGER[0] -->
    // NRF_EGU1->EVENTS_TRIGGERED[0] ---> SystemReset()
    //
    // However, Terbium can't share a RTC timer between tick and watchdog.
    // Because all the COMPARE events of the RTC timer have been used by
    // Alarm driver in Terbium. Watchdog has to use another RTC timer as
    // watchdog timer in Terbium.

#if defined(NRF52811_XXAA)
    NRF_RTC_Type *rtc = NRF_RTC1;
#elif defined(NRF52840_XXAA)
    NRF_RTC_Type *rtc = NRF_RTC2;
#endif

    NRF_PPI->CHENCLR       = PPI_CHENCLR_CH19_Msk;        // Disable the Watchdog PPI channel.
    NRF_EGU1->INTENCLR     = EGU_INTENCLR_TRIGGERED0_Msk; // Disable the EGU interrupt.
    rtc->TASKS_STOP        = 1;                           // Disable RTC.
    rtc->INTENCLR          = RTC_INTENCLR_COMPARE1_Msk;   // Disable RTC COMPARE[1] interrupt.
    rtc->EVENTS_COMPARE[1] = 0;
}

void __cxa_pure_virtual(void)
{
    while (1)
        ;
}

void otSysInit(int argc, char *argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    // Switch to app vector table
    SCB->VTOR = tbBootGetVectorTableAddress();

    nrf5MarbleWatchdogDisable();

    if (gPlatformPseudoResetWasRequested)
    {
        otSysDeinit();
    }

#if ((!SOFTDEVICE_PRESENT) && (NRF52840_XXAA))
    // Enable I-code cache
    NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Enabled;
#elif (DCDC_ENABLE)
    NRF_POWER->DCDCEN = 1;
#endif

#if !OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS && PLATFORM_OPENTHREAD_VANILLA
    mbedtls_platform_set_calloc_free(otHeapCAlloc, otHeapFree);
    mbedtls_platform_setup(NULL);
#endif

    nrf_drv_clock_init();

    tbHalGpioInit();

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    nrf5LogInit();
#endif
#if (__MPU_PRESENT == 1)
    nrf5MpuInit();
#endif
    nrf5FaultInit();
    nrf5AlarmInit();
    nrf5RandomInit();
    if (!gPlatformPseudoResetWasRequested)
    {
#if NRF52840_XXAA
        nrf5CryptoInit();
#endif
    }

    tbWatchdogInit();

    nrf5TransportInit(gPlatformPseudoResetWasRequested);
    nrf5MiscInit();

#if TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
    tbPowerSettingsInit();
    tbWirelessCalInit();
#endif

    nrf5RadioInit();
    nrf5TempInit();

#if PLATFORM_FEM_ENABLE_DEFAULT_CONFIG
    PlatformFemSetConfigParams(&PLATFORM_FEM_DEFAULT_CONFIG);
#endif

    gPlatformPseudoResetWasRequested = false;
}

void otSysDeinit(void)
{
    nrf5TempDeinit();
    nrf5RadioDeinit();
    nrf5MiscDeinit();
    if (!gPlatformPseudoResetWasRequested)
    {
#if NRF52840_XXAA
        nrf5CryptoDeinit();
#endif
    }

    nrf5TransportDeinit(gPlatformPseudoResetWasRequested);
    nrf5RandomDeinit();
    nrf5AlarmDeinit();
    nrf5FaultDeinit();
#if (__MPU_PRESENT == 1)
    nrf5MpuDeinit();
#endif
#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED) || \
    (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL)
    nrf5LogDeinit();
#endif

#if !OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS && PLATFORM_OPENTHREAD_VANILLA
    mbedtls_platform_teardown(NULL);
#endif
}

bool otSysPseudoResetWasRequested(void)
{
    return gPlatformPseudoResetWasRequested;
}

void otSysProcessDrivers(otInstance *aInstance)
{
    tbHalWatchdogRefresh();
    nrf5RadioProcess(aInstance);
    nrf5TransportProcess();
    nrf5TempProcess();
    nrf5AlarmProcess(aInstance);
#if ((OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE == 1) && (TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE == 1))
    tbCoexRadioUnitTestProcess();
#endif
}

__WEAK void otSysEventSignalPending(void)
{
    // Intentionally empty
}
