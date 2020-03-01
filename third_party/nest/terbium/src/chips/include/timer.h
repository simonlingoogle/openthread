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
 *   This file includes the hardware abstraction layer for timer.
 *
 */

#ifndef CHIPS_INCLUDE_TIMER_H__
#define CHIPS_INCLUDE_TIMER_H__

#include <stdbool.h>
#include <stdint.h>

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type represents Timer identifier.
 *
 */
typedef uint8_t tbHalTimer;

/**
 * This function pointer is called when the Timer interrupt is generated.
 *
 * @param[in]  aTimer    Timer indentifier.
 * @param[in]  aContext  A pointer to application-specific context.
 *
 */
typedef void (*tbHalTimerIntCallback)(tbHalTimer aTimer, void *aContext);

/**
 * This function initializes the timer.
 *
 * @param[in] aTimer     Timer indentifier.
 * @param[in] aCallback  A pointer to callback function.
 * @param[in] aContext   A pointer to application-specific context.
 *
 * @retval OT_ERROR_NONE    Initialization was successful.
 * @retval OT_ERROR_FAILED  Failed to initialize the timer.
 *
 */
otError tbHalTimerInit(tbHalTimer aTimer, tbHalTimerIntCallback aCallback, void *aContext);

/**
 * This function initializes the timer.
 *
 * @param[in] aTimer  Timer indentifier.
 *
 */
void tbHalTimerDeinit(tbHalTimer aTimer);

/**
 * This function starts the timer. When the given time has elapsed, timer will
 * invoke the callback function.
 *
 * @param[in] aTimer  Timer indentifier.
 * @param[in] aTime   A time after which the callback funtion will be invoked, in microseconds.
 *
 * @retval OT_ERROR_NONE    Successfully started the timer.
 * @retval OT_ERROR_FAILED  Failed to start the timer.
 *
 */
otError tbHalTimerStart(tbHalTimer aTimer, uint32_t aTimeUs);

/**
 * This function stops the timer and freezes the elapsed time.
 *
 * @param[in] aTimer  Timer indentifier.
 *
 * @retval OT_ERROR_NONE    Successfully stopped the timer.
 * @retval OT_ERROR_FAILED  Failed to stop the timer.
 *
 */
otError tbHalTimerStop(tbHalTimer aTimer);

/**
 * This function returns the number of microseconds elapsed since timer started
 * running. The elapsed time will be frozen when the user calls `tbHalTimerStop()`
 * or the timer interrupt occurs. The elapsed time is set to 0 when user calls
 * `tbHalTimerStart()`.
 *
 * @param[in] aTimer  Timer indentifier.
 *
 * @returns The elapsed time.
 *
 */
uint32_t tbHalTimerGetElapsedTime(tbHalTimer aTimer);

/**
 * This function indicates whether the timer is running.
 *
 * @param[in] aTimeUs  Timer indentifier.
 *
 * @returns true if the timer is running, false otherwise.
 *
 */
bool tbHalTimerIsRunning(tbHalTimer aTimer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_TIMER_H__
