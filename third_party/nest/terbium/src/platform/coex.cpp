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

/*
 * @file
 *   This file implements functions for radio coexistence.
 *
 */

#include <openthread-core-config.h>
#include <terbium-board-config.h>
#include <terbium-config.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#include "chips/include/cmsis.h"
#include "chips/include/gpio.h"
#include "chips/include/timer.h"
#include "platform/coex.h"

#if OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE

#define COEX_GRANT_WAIT_DELAYED_DEADLINE_US 50 ///< Coex grant wait delayed deadline time, in microseconds.
#define COEX_GRANT_WAIT_TIMEOUT_US 25000       ///< Coex grant wait timeout time, in microseconds.

#define COEX_TX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT \
    2 ///< Promote coex priority after n grant deactivation truncated transmissions.
#define COEX_RX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT \
    2 ///< Promote coex priority after n grant deactivation truncated receptions.

#define COEX_INC(v) coexSum(&(v), 1)      ///< Add 1 to the given value.
#define COEX_ADD(s, v) coexSum(&(s), (v)) ///< Add two values.

#if TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
static void coexUnitTestInit(void);
#endif

/**
 * This structure containing coex gpio pin assignments.
 *
 */
typedef struct CoexConfig
{
    tbHalGpioPin mRequestPin;  ///< Coex request pin.
    tbHalGpioPin mPriorityPin; ///< Coex priority pin.
    tbHalGpioPin mGrantPin;    ///< Coex grant pin.
} CoexConfig;

/**
 * This enumeration defines the coex states.
 *
 */
typedef enum CoexState
{
    kCoexStateUninitialized   = 0, ///< Coex is uninitialized.
    kCoexStateDisabled        = 1, ///< Coex is disabled.
    kCoexStateWaitingForRadio = 2, ///< Coex is ready but radio driver is not enabled.
    kCoexStateEnabled         = 3, ///< Coex is enabled.
} CoexState;

/**
 * This enumeration defines the pending coex events.
 *
 * An event will be pending when it is detected while coex is in
 * a critical section.  A coex event that happens during a coex
 * critical section will be handled upon exit from the critical
 * section. By design only one event will ever be pending at a
 * time so the event "queue" only needs be one element in length.
 *
 */
typedef enum CoexPendingEvent
{
    kCoexPendingEventNone             = 0, ///< No pending event.
    kCoexPendingEventCoexEnabled      = 1, ///< Coex enable event.
    kCoexPendingEventGrantActivated   = 2, ///< Grant activation event.
    kCoexPendingEventGrantDeactivated = 3, ///< Grant deactivation event.
} CoexPendingEvent;

/**
 * This enumeration defines the coex TX/RX radio frame states.
 *
 */
typedef enum CoexRadioState
{
    kCoexRadioStateIdle            = 0, ///< Idle state.
    kCoexRadioStateWaitingForGrant = 1, ///< Radio driver is waiting for grant.
    kCoexRadioStateRadioInProcess  = 2, ///< Radio driver is processing TX/RX frame.
} CoexRadioState;

static const CoexConfig sCoexConfig = {
    .mRequestPin  = TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN,
    .mPriorityPin = TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN,
    .mGrantPin    = TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN,
};

static volatile otRadioCoexMetrics sCoexMetrics;
static volatile CoexState          sCoexState    = kCoexStateUninitialized;
static volatile CoexRadioState     sRxFrameState = kCoexRadioStateIdle;
static volatile CoexRadioState     sTxFrameState = kCoexRadioStateIdle;
static volatile CoexPendingEvent   sPendingEvent = kCoexPendingEventNone;

static volatile bool           sRadioDriverStarted           = false;
static volatile bool           sRadioDriverInCriticalSection = false;
static volatile bool           sRequestPriorityIsHigh        = false;
static volatile bool           sGrantActive                  = false;
static volatile bool           sTxAckFrameInProgress         = false;
static volatile uint32_t       sRxGrantDeactivationCount     = 0;
static volatile uint32_t       sTxGrantDeactivationCount     = 0;
static volatile uint8_t *      sTxFrame                      = NULL;
static volatile uint32_t       sGrantDelay                   = 0;
static volatile uint32_t       sRxWaitTimeAdjustment         = 0;
static volatile uint32_t       sTxWaitTimeAdjustment         = 0;
static volatile tbHalGpioValue sGrantPinValue                = TB_HAL_GPIO_VALUE_LOW;
static volatile tbHalGpioValue sGrantPinValuePrevious        = TB_HAL_GPIO_VALUE_LOW;

static void coexGrantPinHandler(tbHalGpioPin aPin);

static inline void coexSum(volatile uint32_t *aValueA, uint32_t aValueB)
{
    if (!sCoexMetrics.mStopped)
    {
        uint32_t result = *aValueA + aValueB;

        if (result >= *aValueA)
        {
            *aValueA = result;
        }
        else
        {
            sCoexMetrics.mStopped = true;
        }
    }
}

static inline void coexActivateRequest(void)
{
    bool priorityIsHigh = sRequestPriorityIsHigh ||
                          (sRxGrantDeactivationCount >= COEX_RX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT) ||
                          (sTxGrantDeactivationCount >= COEX_TX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT);

    // Activate the request and priority lines.
    tbHalGpioSetValue(sCoexConfig.mPriorityPin, (priorityIsHigh) ? TB_HAL_GPIO_VALUE_HIGH : TB_HAL_GPIO_VALUE_LOW);
    tbHalGpioSetValue(sCoexConfig.mRequestPin, TB_HAL_GPIO_VALUE_HIGH);
}

static inline void coexDeactivateRequest(void)
{
    // Deactivate the request and priority lines.
    tbHalGpioSetValue(sCoexConfig.mRequestPin, TB_HAL_GPIO_VALUE_LOW);
    tbHalGpioSetValue(sCoexConfig.mPriorityPin, TB_HAL_GPIO_VALUE_LOW);
}

static void coexGrantActivated(void)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);
    VerifyOrExit(!sGrantActive);

    // Indicate that the grant has become active.
    sGrantActive = true;

    // Process coex state for any rx frames.
    if (sRxFrameState == kCoexRadioStateWaitingForGrant)
    {
        // Move the coex rx frame state to in progress.
        sRxFrameState = kCoexRadioStateRadioInProcess;

        if (sGrantDelay > COEX_GRANT_WAIT_DELAYED_DEADLINE_US)
        {
            COEX_INC(sCoexMetrics.mNumRxDelayedGrant);
        }

        COEX_INC(sCoexMetrics.mNumRxGrantWaitActivated);
        COEX_ADD(sCoexMetrics.mAvgRxRequestToGrantTime, sGrantDelay - sRxWaitTimeAdjustment);
    }

    // Process any pending tx frames.
    if (sTxFrameState == kCoexRadioStateWaitingForGrant)
    {
        // Don't change sTxFrameState here, it will be changed by tbCoexRadioTxRequest()
        // which is called by tbCoexRadioCsmaCaTransmitStart()
        tbCoexRadioCsmaCaTransmitStart((const uint8_t *)sTxFrame);

        if (sGrantDelay > COEX_GRANT_WAIT_DELAYED_DEADLINE_US)
        {
            COEX_INC(sCoexMetrics.mNumTxDelayedGrant);
        }

        COEX_INC(sCoexMetrics.mNumTxGrantWaitActivated);
        COEX_ADD(sCoexMetrics.mAvgTxRequestToGrantTime, sGrantDelay - sTxWaitTimeAdjustment);
    }

exit:
    return;
}

static void coexGrantDeactivated(void)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);
    VerifyOrExit(sGrantActive);

    // Indicate that the grant has become inactive.
    sGrantActive = false;

    if (sRadioDriverStarted)
    {
        if (sRxFrameState == kCoexRadioStateRadioInProcess)
        {
            if (sRxGrantDeactivationCount < COEX_RX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT)
            {
                COEX_INC(sRxGrantDeactivationCount);
            }

            if (sTxAckFrameInProgress)
            {
                // Restart the radio to abort anything it was doing.
                tbCoexRadioRestart();
            }
        }

        if (sTxFrameState == kCoexRadioStateRadioInProcess)
        {
            if (sTxGrantDeactivationCount < COEX_TX_GRANT_DEACTIVATION_PRIORITY_PROMOTION_COUNT)
            {
                COEX_INC(sTxGrantDeactivationCount);
            }

            // Restart the radio to abort anything it was doing.
            tbCoexRadioRestart();
        }
    }

    // Deactivate the request and priority lines.
    coexDeactivateRequest();

    if (sRxFrameState != kCoexRadioStateIdle)
    {
        COEX_INC(sCoexMetrics.mNumRxGrantDeactivatedDuringRequest);
    }

    if (sTxFrameState != kCoexRadioStateIdle)
    {
        COEX_INC(sCoexMetrics.mNumTxGrantDeactivatedDuringRequest);
    }

exit:
    return;
}

static void coexEnterEnabledState(void)
{
    // Move the coex state to enabled.
    sCoexState = kCoexStateEnabled;

    // Reset coex pending event.
    sPendingEvent = kCoexPendingEventNone;

    // Reset rx frame state variables.
    sRxFrameState             = kCoexRadioStateIdle;
    sRxGrantDeactivationCount = 0;

    // Reset ack frame state variable.
    sTxAckFrameInProgress = false;

    // Reset tx frame state variables.
    sTxFrameState             = kCoexRadioStateIdle;
    sTxGrantDeactivationCount = 0;
    sTxFrame                  = NULL;

    // Reset all coex stats.
    memset((void *)&sCoexMetrics, 0, sizeof(sCoexMetrics));

    // Request and priority pins are low outputs.
    tbHalGpioOutputRequest(sCoexConfig.mRequestPin, TB_HAL_GPIO_VALUE_LOW);
    tbHalGpioOutputRequest(sCoexConfig.mPriorityPin, TB_HAL_GPIO_VALUE_LOW);

    // Enable the grant interrupt.
    // When 'tbHalGpioIntRequest()' enables the interrupt it checks the current state
    // of the gpio and then configures the sense to be the opposite of the current
    // state.  So, if the grant line is active when we enable the grant line gpio
    // interrupt we will not receive an interrupt until the grant line is deactivated.
    // When we enter the driver enabled state we check if the grant line is active
    // and initialize the grant state variables accordingly.
    // It's possible that there was actually a change of the grant line state after
    // we enabled the interrupt but before we read the state of the pin.  If this
    // happens then the 'coexGrantPinHandler()' will fire when we re-enable
    // interrupts and the grant interrupt will be treated as a glitch and there will
    // be no other side effects.
    tbHalInterruptDisable();

    // Configure the grant gpio interrupt.
    tbHalGpioIntRequest(sCoexConfig.mGrantPin, TB_HAL_GPIO_PULL_NONE, TB_HAL_GPIO_INT_MODE_BOTH_EDGES,
                        coexGrantPinHandler);

    // Update grant state variables.
    sGrantPinValue         = tbHalGpioGetValue(sCoexConfig.mGrantPin);
    sGrantActive           = (sGrantPinValue == TB_HAL_GPIO_VALUE_HIGH) ? true : false;
    sGrantPinValuePrevious = sGrantPinValue;

    tbHalInterruptEnable();
}

static void coexEnterWaitingForRadioState(bool transmit_pending_frame)
{
    // The coex driver can enter the waiting for radio state because the radio
    // was disabled or because the coex client disabled coex. The transmit
    // pending frame will be set to true if we are changing states due to the
    // coex client disabling coex. It will be false if we are changing state
    // due to the radio being disabled.

    sCoexState = kCoexStateWaitingForRadio;

    // Disable the grant interrupt.
    tbHalGpioIntRelease(sCoexConfig.mGrantPin);

    // All coex pins are inputs.
    tbHalGpioInputRequest(sCoexConfig.mRequestPin, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioInputRequest(sCoexConfig.mPriorityPin, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioInputRequest(sCoexConfig.mRequestPin, TB_HAL_GPIO_PULL_NONE);

    // If there is a pending tx frame then handle it.
    if (sTxFrameState == kCoexRadioStateWaitingForGrant)
    {
        tbHalTimerStop(TERBIUM_BOARD_CONFIG_COEX_TIMER);

        if (transmit_pending_frame)
        {
            tbCoexRadioCsmaCaTransmitStart((const uint8_t *)sTxFrame);
        }
        else
        {
            // If we are being moved to this state by the radio driver then we
            // should fail any pending frames.
            tbCoexRadioTransmitFailed((const uint8_t *)sTxFrame, TB_COEX_RADIO_ERROR_RADIO_DISABLED);
        }
    }
}

static uint32_t coexStartGrantWaitTimeoutTimer(void)
{
    // The grant wait timer is used to measure the delay from a coex request to
    // a coex grant and also to implement a grant wait timeout.  It is possible
    // that the timer is already running when this function is called.

    uint32_t elapsedTime = 0;

    if (tbHalTimerIsRunning(TERBIUM_BOARD_CONFIG_COEX_TIMER))
    {
        tbHalTimerStop(TERBIUM_BOARD_CONFIG_COEX_TIMER);
        elapsedTime = tbHalTimerGetElapsedTime(TERBIUM_BOARD_CONFIG_COEX_TIMER);
    }

    elapsedTime = (elapsedTime < COEX_GRANT_WAIT_TIMEOUT_US) ? elapsedTime : 0;

    tbHalTimerStart(TERBIUM_BOARD_CONFIG_COEX_TIMER, COEX_GRANT_WAIT_TIMEOUT_US - elapsedTime);

    return elapsedTime;
}

static void coexGrantTimeoutHandler(tbHalTimer aTimer, void *aContext)
{
    OT_UNUSED_VARIABLE(aTimer);
    OT_UNUSED_VARIABLE(aContext);

    // If there is a pending tx frame then fail it.
    if (sTxFrameState == kCoexRadioStateWaitingForGrant)
    {
        // Fail any pending frames.
        tbCoexRadioTransmitFailed((const uint8_t *)sTxFrame, TB_COEX_RADIO_ERROR_GRANT_TIMEOUT);

        COEX_INC(sCoexMetrics.mNumTxGrantWaitTimeout);
        COEX_ADD(sCoexMetrics.mAvgTxRequestToGrantTime, COEX_GRANT_WAIT_TIMEOUT_US - sTxWaitTimeAdjustment);
    }

    if (sRxFrameState == kCoexRadioStateWaitingForGrant)
    {
        COEX_INC(sCoexMetrics.mNumRxGrantWaitTimeout);
        COEX_ADD(sCoexMetrics.mAvgRxRequestToGrantTime, COEX_GRANT_WAIT_TIMEOUT_US - sRxWaitTimeAdjustment);
    }
}

int coexEnable(void)
{
    VerifyOrExit(sCoexState == kCoexStateDisabled);

    sCoexState = kCoexStateWaitingForRadio;

    if (sRadioDriverStarted)
    {
        if (sRadioDriverInCriticalSection)
        {
            sPendingEvent = kCoexPendingEventCoexEnabled;
        }
        else
        {
            // Restart the radio to put it into a known state.
            tbCoexRadioRestart();
            coexEnterEnabledState();
        }
    }

exit:
    return 0;
}

int coexDisable(void)
{
    switch (sCoexState)
    {
    case kCoexStateEnabled:
        coexEnterWaitingForRadioState(true);
        // fall-through
    case kCoexStateWaitingForRadio:
        sCoexState = kCoexStateDisabled;
        break;
    case kCoexStateUninitialized:
    case kCoexStateDisabled:
    default:
        break;
    }

    return 0;
}

void tbCoexRadioInit(void)
{
    VerifyOrExit(sCoexState == kCoexStateUninitialized);

    sCoexState = kCoexStateDisabled;

    // Priority is low by default.
    sRequestPriorityIsHigh = false;

    // These variables track radio driver state.
    sRadioDriverStarted           = false;
    sRadioDriverInCriticalSection = false;

    tbHalGpioInputRequest(sCoexConfig.mRequestPin, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioInputRequest(sCoexConfig.mPriorityPin, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioInputRequest(sCoexConfig.mRequestPin, TB_HAL_GPIO_PULL_NONE);

    tbHalTimerInit(TERBIUM_BOARD_CONFIG_COEX_TIMER, coexGrantTimeoutHandler, NULL);

#if TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
    coexUnitTestInit();
#endif

    coexEnable();

exit:
    return;
}

otError otPlatRadioSetCoexEnabled(otInstance *aInstance, bool aEnabled)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (aEnabled)
    {
        coexEnable();
    }
    else
    {
        coexDisable();
    }

    return OT_ERROR_NONE;
}

bool otPlatRadioIsCoexEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return (sCoexState == kCoexStateEnabled) || (sCoexState == kCoexStateWaitingForRadio);
}

void otPlatCoexSetPriority(otInstance *aInstance, bool aPriority)
{
    OT_UNUSED_VARIABLE(aInstance);
    sRequestPriorityIsHigh = aPriority;
}

bool otPlatCoexGetPriority(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return sRequestPriorityIsHigh;
}

otError otPlatRadioGetCoexMetrics(otInstance *aInstance, otRadioCoexMetrics *aCoexMetrics)
{
    otError error = OT_ERROR_NONE;
    bool    stopped;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aCoexMetrics != NULL, error = OT_ERROR_INVALID_ARGS);

    // Make sure stats collection is stopped before we make a copy
    // since the stopped state can be set in a stats sum operation
    // and since the stat sum operation can happen inside of an
    // interrupt, we need the read/write operations to be atomic
    // to guarantee the integrity of the stopped state returned.
    stopped = __sync_lock_test_and_set(&sCoexMetrics.mStopped, true);

    if (sCoexMetrics.mNumRxRequest > 0)
    {
        sCoexMetrics.mAvgRxRequestToGrantTime /= sCoexMetrics.mNumRxRequest;
    }

    if (sCoexMetrics.mNumTxRequest > 0)
    {
        sCoexMetrics.mAvgTxRequestToGrantTime /= sCoexMetrics.mNumTxRequest;
    }

    memcpy(aCoexMetrics, (void *)&sCoexMetrics, sizeof(sCoexMetrics));

    // Re-write the actual stopped state.
    aCoexMetrics->mStopped = stopped;

    // This memset will also reset the state of stat collection to not stopped.
    memset((void *)&sCoexMetrics, 0, sizeof(sCoexMetrics));

exit:
    return error;
}

void tbCoexRadioStart(void)
{
    sRadioDriverStarted = true;

    if (sCoexState == kCoexStateWaitingForRadio)
    {
        coexEnterEnabledState();
    }
}

void tbCoexRadioStop(void)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);

    coexEnterWaitingForRadioState(false);

exit:
    sRadioDriverStarted = false;
}

bool tbCoexRadioRxRequest(void)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);

    // Make the rx request processing atomic.
    tbHalInterruptDisable();

    coexActivateRequest();

    if (sRxFrameState == kCoexRadioStateIdle)
    {
        COEX_INC(sCoexMetrics.mNumRxRequest);
    }

    if (sGrantActive)
    {
        // If grant is active then move to the rx state to in progress.
        if (sRxFrameState == kCoexRadioStateIdle)
        {
            COEX_INC(sCoexMetrics.mNumRxGrantImmediate);
        }

        sRxFrameState = kCoexRadioStateRadioInProcess;
    }
    else
    {
        // If grant is not active then start a timeout timer and move the rx
        // state to waiting for grant.
        sRxWaitTimeAdjustment = coexStartGrantWaitTimeoutTimer();

        if (sRxFrameState == kCoexRadioStateIdle)
        {
            COEX_INC(sCoexMetrics.mNumRxGrantWait);
        }

        sRxFrameState = kCoexRadioStateWaitingForGrant;
    }

    tbHalInterruptEnable();

exit:
    // Always return true so as to allow the radio to attempt to continue to
    // receive the RX frame even if the request was not granted.
    return true;
}

bool tbCoexTxAckRequest(void)
{
    bool retval = false;

    VerifyOrExit(sCoexState == kCoexStateEnabled, retval = true);
    VerifyOrExit(sRxFrameState == kCoexRadioStateRadioInProcess);

    // Make the ack request processing atomic.
    tbHalInterruptDisable();

    if (sGrantActive)
    {
        // If grant is active then move to the ack state to in progress.
        sTxAckFrameInProgress = true;
        retval                = true;
    }

    tbHalInterruptEnable();

exit:
    return retval;
}

bool tbCoexRadioTxRequest(const uint8_t *aTxFrame)
{
    bool retval = true;

    VerifyOrExit(sCoexState == kCoexStateEnabled);

    // Make the tx request processing atomic.
    tbHalInterruptDisable();

    coexActivateRequest();

    if (sTxFrameState == kCoexRadioStateIdle)
    {
        COEX_INC(sCoexMetrics.mNumTxRequest);
    }

    if (sGrantActive)
    {
        // If grant is active then move to the tx state to in progress.
        if (sTxFrameState == kCoexRadioStateIdle)
        {
            COEX_INC(sCoexMetrics.mNumTxGrantImmediate);
        }

        sTxFrameState = kCoexRadioStateRadioInProcess;
    }
    else
    {
        // If grant is not active then start a timeout timer, save the pointer
        // to the tx frame and move the rx state to waiting for grant.
        sTxWaitTimeAdjustment = coexStartGrantWaitTimeoutTimer();

        if (sTxFrameState == kCoexRadioStateIdle)
        {
            COEX_INC(sCoexMetrics.mNumTxGrantWait);
        }

        // The grant line is not active so coex is going to take ownership
        // of this tx frame which is currently owned by the CSMA/CA module of
        // the radio driver. This call will cause the csma_ca module of the
        // radio driver to reset it's state machine and give up ownership of
        // the tx frame.
        tbCoexRadioCsmaCaTransmitStop();

        sTxFrameState = kCoexRadioStateWaitingForGrant;
        sTxFrame      = (volatile uint8_t *)aTxFrame;
        retval        = false;
    }

    tbHalInterruptEnable();

exit:
    return retval;
}

bool tbCoexRadioIsGrantActive(void)
{
    bool retval = true;

    VerifyOrExit(sCoexState == kCoexStateEnabled);

    retval = sGrantActive;

exit:
    return retval;
}

void tbCoexRadioCriticalSectionEnter(void)
{
    sRadioDriverInCriticalSection = true;
}

void tbCoexRadioCriticalSectionExit(void)
{
    // The radio driver will occasionally call tbCoexRadioCriticalSectionExit()
    // more than once for a single call to tbCoexRadioCriticalSectionEnter().
    VerifyOrExit(sRadioDriverInCriticalSection == true);

    if (sCoexState == kCoexStateEnabled)
    {
        // Grant activation/deactivation processing must be atomic.
        tbHalInterruptDisable();

        switch (sPendingEvent)
        {
        case kCoexPendingEventGrantActivated:
            coexGrantActivated();
            break;
        case kCoexPendingEventGrantDeactivated:
            coexGrantDeactivated();
            break;
        case kCoexPendingEventNone:
            // Do nothing.
            break;
        default:
            assert(false);
            break;
        }

        sPendingEvent                 = kCoexPendingEventNone;
        sRadioDriverInCriticalSection = false;

        tbHalInterruptEnable();
    }
    else
    {
        CoexPendingEvent pending_event = sPendingEvent;

        sPendingEvent                 = kCoexPendingEventNone;
        sRadioDriverInCriticalSection = false;

        if ((sCoexState == kCoexStateWaitingForRadio) && (pending_event == kCoexPendingEventCoexEnabled))
        {
            // Restart the radio to put it into a known state.
            tbCoexRadioRestart();
            coexEnterEnabledState();
        }
    }

exit:
    return;
}

void tbCoexRadioRxEnded(bool aSuccess)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);

    if (sRxFrameState != kCoexRadioStateIdle)
    {
        if (aSuccess)
        {
            sRxGrantDeactivationCount = 0;
        }

        if (sRxFrameState == kCoexRadioStateWaitingForGrant)
        {
            COEX_INC(sCoexMetrics.mNumRxGrantNone);
        }

        // Release request if not tx in progress. This handles an unlikely
        // possibility but depending on system latencies it might be possible
        // to have both an rx frame in progress and a tx frame in progress.
        // If this happens we do not want to deactivate the request line until
        // we receive done notifications for both frames.
        if (sTxFrameState == kCoexRadioStateIdle)
        {
            coexDeactivateRequest();

            if (sRxFrameState == kCoexRadioStateWaitingForGrant)
            {
                // It is possible to receive a frame even if grant never became
                // active. If that happens then the grant wait timeout timer
                // will still be running so we need to stop it here.
                tbHalTimerStop(TERBIUM_BOARD_CONFIG_COEX_TIMER);
            }
        }

        sRxFrameState         = kCoexRadioStateIdle;
        sTxAckFrameInProgress = false;
    }

exit:
    return;
}

void tbCoexRadioTxEnded(bool aSuccess)
{
    VerifyOrExit(sCoexState == kCoexStateEnabled);

    if (sTxFrameState != kCoexRadioStateIdle)
    {
        sTxFrameState = kCoexRadioStateIdle;
        sTxFrame      = NULL;

        if (aSuccess)
        {
            sTxGrantDeactivationCount = 0;
        }

        // Release request if not rx in progress. This handles an unlikely
        // possibility but depending on system latencies it might be possible
        // to have both an rx frame in progress and a tx frame in progress.
        // If this happens we do not want to deactivate the request line
        // until we receive done notifications for both frames.
        if (sRxFrameState == kCoexRadioStateIdle)
        {
            coexDeactivateRequest();
        }
    }

exit:
    return;
}

static void coexGrantPinHandler(tbHalGpioPin aPin)
{
    // The priority of the interrupt that calls this handler must be equal to
    // the radio interrupt priority. If it is higher then we might not be able
    // to enter critical sections of the driver. If it is lower then the driver
    // may change states during execution of this ISR.

    OT_UNUSED_VARIABLE(aPin);

    // Get the current state of grant.
    sGrantPinValue = tbHalGpioGetValue(sCoexConfig.mGrantPin);

    VerifyOrExit(sCoexState == kCoexStateEnabled);

    if (sGrantPinValue == sGrantPinValuePrevious)
    {
        // We missed an edge or we had a glitch track it but there's nothing
        // that needs to be done to recover so return.
        COEX_INC(sCoexMetrics.mNumGrantGlitch);
        ExitNow();
    }

    tbHalTimerStop(TERBIUM_BOARD_CONFIG_COEX_TIMER);
    sGrantDelay = tbHalTimerGetElapsedTime(TERBIUM_BOARD_CONFIG_COEX_TIMER);

    // Handle transitions.
    if (sGrantPinValue)
    {
        if (sRadioDriverInCriticalSection)
        {
            if (sPendingEvent == kCoexPendingEventNone)
            {
                sPendingEvent = kCoexPendingEventGrantActivated;
            }
            else
            {
                sPendingEvent = kCoexPendingEventNone;
            }
        }
        else
        {
            coexGrantActivated();
        }
    }
    else
    {
        if (sRadioDriverInCriticalSection)
        {
            if (sPendingEvent == kCoexPendingEventNone)
            {
                sPendingEvent = kCoexPendingEventGrantDeactivated;
            }
            else
            {
                sPendingEvent = kCoexPendingEventNone;
            }
        }
        else
        {
            coexGrantDeactivated();
        }
    }

exit:
    sGrantPinValuePrevious = sGrantPinValue;
}

#if TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
#include <openthread/platform/alarm-milli.h>

/**
 ****************************************************************************
 * Coex Unit Test
 *
 * When debugging coex issue on real products, the failure of coex many cause
 * the radio driver enter assert state. Then the spinel interface is broken
 * and the posix host will reboot the RCP chip automatically. It is hard for
 * us to debug the coex.
 *
 * This unit test section contains code used to develop and debug 3-wire coex
 * on a dev board. It implements a coex aribiter emulator and a Wifi emulator
 * and uses a gpio output to simulate the coex arbiter grant signal. The
 * simulated grant signal needs to be jumpered to the lowpan radio coex grant
 * line under test. It also uses 2 other gpio inputs to indicate whether the
 * lowpan radio coex driver is making a request and the current coex priority.
 *
 * When unit test is enabled, the Wifi emulator periodically send packets.
 * Before sending a packet, the Wifi emulator requests radio resource from
 * coex arbiter. After the packet is send done, it will release the radio
 * resource to aribiter. The coex aribter decides whether to allocate radio
 * resources immediately or after the current PHY releases resources based
 * on the aribiter internal priorities.
 *
 * Coex unit test pin connection map:
 * TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN  ----> TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN
 * TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN ----> TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN
 * TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN     <---- TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN
 *
 ****************************************************************************
 */

#define COEX_MAX_DELTA_TIME (0xffffffff >> 1) ///< The maximum delta time.
#define COEX_WIFI_SENDING_TIME_MS 50          ///< Wifi emulator sending packet duration, in millisecond.
#define COEX_WIFI_IDLE_TIME_MS 1000           ///< Wifi emulator idle duration, in millisecond.

/**
 * This enumeration defines the Coex PHY types.
 *
 */
typedef enum
{
    kCoexPhyNone    = 0, ///< None.
    kCoexPhyLowpan  = 1, ///< Lowpan.
    kCoexPhyWifi    = 2, ///< Wifi.
    kCoexNumPhyType = 3, ///< Number of PHYs.
} CoexPhy;

/**
 * This enumeration defines the Wifi states.
 *
 */
typedef enum
{
    kStateIdle         = 0, ///< Idle.
    KStateWaitGrant    = 1, ///< Wifi is waiting for frant.
    KStateSendingFrame = 2, ///< Wifi is sending packets.
} CoexWifiState;

/**
 * This enumeration defines the Coex request priorities.
 *
 */
typedef enum
{
    kRequestPriorityLow  = 0, ///< High priority.
    kRequestPriorityHigh = 1, ///< Low priority.
} CoexRequestPriority;

/**
 * This enumeration defines the Coex arbiter internal priorities.
 *
 */
typedef struct CoexArbiterPriority
{
    uint8_t mPriorityLow;            ///< High priority.
    uint8_t mPriorityHigh;           ///< Low priority.
    uint8_t mPriorityDeltaInProcess; ///< Delta priority in process.
} CoexArbiterPriority;

static const CoexArbiterPriority    sArbiterPriorities[kCoexNumPhyType] = {{0, 0, 0}, {5, 15, 5}, {1, 8, 4}};
static volatile CoexPhy             sArbiterCurPhy;
static volatile CoexPhy             sArbiterPendingPhy;
static volatile CoexRequestPriority sArbiterCurRequestPriority;
static volatile CoexRequestPriority sArbiterPendingRequestPriority;

static CoexWifiState           sWifiState;
static CoexRequestPriority     sWifiRequestPriority;
static uint32_t                sWifiTimerFireTime;
static bool                    sWifiTimerIsRunning;
static volatile tbHalGpioValue sWifiGrantPinValue;
static volatile tbHalGpioValue sWifiGrantPinValuePrevious;

static inline uint8_t coexGetArbiterPriority(CoexPhy aPhy, CoexRequestPriority aPriority)
{
    return (aPriority == kRequestPriorityHigh) ? sArbiterPriorities[aPhy].mPriorityHigh
                                               : sArbiterPriorities[aPhy].mPriorityLow;
}

static inline uint8_t coexGetArbiterPriorityInProcess(CoexPhy aPhy, CoexRequestPriority aPriority)
{
    uint8_t priority;

    if (aPriority == kRequestPriorityHigh)
    {
        priority = sArbiterPriorities[aPhy].mPriorityHigh + sArbiterPriorities[aPhy].mPriorityDeltaInProcess;
    }
    else
    {
        priority = sArbiterPriorities[aPhy].mPriorityLow + sArbiterPriorities[aPhy].mPriorityDeltaInProcess;
    }

    return priority;
}

static inline void coexWifiSetGrantPinValue(tbHalGpioValue aValue)
{
    sWifiGrantPinValue = aValue;
}

static inline void coexArbiterSetGrantPinValue(CoexPhy aPhy, tbHalGpioValue aValue)
{
    if (aPhy == kCoexPhyLowpan)
    {
        tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN, aValue);
    }
    else
    {
        coexWifiSetGrantPinValue(aValue);
    }
}

static inline void coexArbiterActiveGrantPin(CoexPhy aPhy, CoexRequestPriority aPriority)
{
    sArbiterCurPhy             = aPhy;
    sArbiterCurRequestPriority = aPriority;

    if (aPhy == kCoexPhyNone)
    {
        // No one is using the radio resource.
        coexArbiterSetGrantPinValue(kCoexPhyWifi, TB_HAL_GPIO_VALUE_HIGH);
        coexArbiterSetGrantPinValue(kCoexPhyLowpan, TB_HAL_GPIO_VALUE_HIGH);
    }
    else if (aPhy == kCoexPhyLowpan)
    {
        // Lowpan got the radio resource.
        coexArbiterSetGrantPinValue(kCoexPhyWifi, TB_HAL_GPIO_VALUE_LOW);
        coexArbiterSetGrantPinValue(kCoexPhyLowpan, TB_HAL_GPIO_VALUE_HIGH);
    }
    else if (aPhy == kCoexPhyWifi)
    {
        // Wifi got the radio resource.
        coexArbiterSetGrantPinValue(kCoexPhyLowpan, TB_HAL_GPIO_VALUE_LOW);
        coexArbiterSetGrantPinValue(kCoexPhyWifi, TB_HAL_GPIO_VALUE_HIGH);
    }
}

static bool coexArbiterRequest(CoexPhy aPhy, CoexRequestPriority aPriority)
{
    bool    retval = false;
    uint8_t curPriority;
    uint8_t nextPriority;

    tbHalInterruptDisable();

    curPriority  = coexGetArbiterPriorityInProcess(sArbiterCurPhy, sArbiterCurRequestPriority);
    nextPriority = coexGetArbiterPriority(aPhy, aPriority);

    if (nextPriority <= curPriority)
    {
        // Add the PHY to the pending list.
        sArbiterPendingPhy             = aPhy;
        sArbiterPendingRequestPriority = aPriority;
        ExitNow();
    }

    coexArbiterActiveGrantPin(aPhy, aPriority);

    retval = true;

exit:
    tbHalInterruptEnable();
    return retval;
}

static void coexArbiterRelease(CoexPhy aPhy)
{
    tbHalInterruptDisable();

    if (sArbiterPendingPhy == aPhy)
    {
        // The PHY never gets the radio resource, just remove it from the pending list.
        sArbiterPendingPhy = kCoexPhyNone;
    }
    else if (sArbiterCurPhy == aPhy)
    {
        // Current PHY releases the radio resource. Arbiter allocates radio resource to the pending PHY.
        coexArbiterActiveGrantPin(sArbiterPendingPhy, sArbiterPendingRequestPriority);
        sArbiterPendingPhy = kCoexPhyNone;
    }

    tbHalInterruptEnable();
}

static void coexWifiStartTimer(uint32_t aTimeUs)
{
    // Some chips don't have extra hardware timers. Here we use a software timer.
    sWifiTimerFireTime  = otPlatAlarmMilliGetNow() + aTimeUs;
    sWifiTimerIsRunning = true;
}

static void coexWifiGrantPinHandler(void)
{
    if (sWifiGrantPinValue)
    {
        // Wifi grant is actived.
        if (sWifiState == kStateIdle)
        {
            // Release radio resource to arbiter.
            coexArbiterRelease(kCoexPhyWifi);
        }
        else if (sWifiState == KStateWaitGrant)
        {
            // Enter sending state.
            sWifiState = KStateSendingFrame;
        }
    }
    else
    {
        // Wifi grant is deactived.
        if (sWifiState == KStateSendingFrame)
        {
            // Abort sending frame.
            sWifiState = KStateWaitGrant;
        }
    }
}

static void coexWifiTimerHandler(void)
{
    tbHalGpioSetValue(TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN, TB_HAL_GPIO_VALUE_HIGH);

    if (sWifiState == kStateIdle)
    {
        // Wifi is going to send a packet, request the radio resource first.
        if (coexArbiterRequest(kCoexPhyWifi, sWifiRequestPriority))
        {
            // Wifi grant is active. Enter sending frame state.
            sWifiState           = KStateSendingFrame;
            sWifiRequestPriority = kRequestPriorityLow;
        }
        else
        {
            sWifiState = KStateWaitGrant;
        }

        coexWifiStartTimer(COEX_WIFI_SENDING_TIME_MS);
    }
    else if ((sWifiState == KStateSendingFrame) || (sWifiState == KStateWaitGrant))
    {
        // Release radio resource to arbiter.
        coexArbiterRelease(kCoexPhyWifi);

        if (sWifiState == KStateWaitGrant)
        {
            // Failed to grant the radio resource, increase the Wifi request priority.
            sWifiRequestPriority = kRequestPriorityHigh;
        }

        coexWifiStartTimer(COEX_WIFI_IDLE_TIME_MS);
        sWifiState = kStateIdle;
    }
}

static void coexLowpanRequestPinHandler(tbHalGpioPin aPin)
{
    OT_UNUSED_VARIABLE(aPin);

    if (tbHalGpioGetValue(TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN))
    {
        CoexRequestPriority priority = tbHalGpioGetValue(TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN)
                                           ? kRequestPriorityHigh
                                           : kRequestPriorityLow;

        // Request radio resource from arbiter.
        coexArbiterRequest(kCoexPhyLowpan, priority);
    }
    else
    {
        // Release radio resource to arbiter.
        coexArbiterRelease(kCoexPhyLowpan);
    }
}

static void coexUnitTestInit(void)
{
    // Initializes the pins of the arbiter.
    tbHalGpioInputRequest(TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioIntRequest(TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN, TB_HAL_GPIO_PULL_PULLUP,
                        TB_HAL_GPIO_INT_MODE_BOTH_EDGES, coexLowpanRequestPinHandler);
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN, TB_HAL_GPIO_VALUE_HIGH);

    sWifiGrantPinValuePrevious = sWifiGrantPinValue;
    coexWifiStartTimer(COEX_WIFI_IDLE_TIME_MS);
}

void tbCoexRadioUnitTestProcess(void)
{
    uint32_t now = otPlatAlarmMilliGetNow();

    if (sWifiGrantPinValuePrevious != sWifiGrantPinValue)
    {
        sWifiGrantPinValuePrevious = sWifiGrantPinValue;
        coexWifiGrantPinHandler();
    }

    if (sWifiTimerIsRunning && ((now - sWifiTimerFireTime) < COEX_MAX_DELTA_TIME))
    {
        sWifiTimerIsRunning = false;
        coexWifiTimerHandler();
    }
}
#endif // TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
#endif // OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
