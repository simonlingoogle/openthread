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

#ifndef CHIPS_INCLUDE_WATCHDOG_H__
#define CHIPS_INCLUDE_WATCHDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method initializes the watchdog and enables the watchdog by default.
 *
 */
void tbWatchdogInit(void);

/**
 * This method enables the watchdog.
 *
 * @param[in] aEnabled   TRUE to enable the watchdog, FALSE otherwise.
 *
 */
void tbHalWatchdogSetEnabled(bool aEnabled);

/**
 * This methid indicates whether the watchdog was enabled.
 *
 * @returns TRUE if the watchdog was enabled, FALSE otherwise.
 *
 */
bool tbHalWatchdogIsEnabled(void);

/**
 * This methid refresh the watchdog timer counter.
 *
 */
void tbHalWatchdogRefresh(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_WATCHDOG_H__
