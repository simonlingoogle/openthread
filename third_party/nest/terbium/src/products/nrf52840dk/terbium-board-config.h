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

/**
 * @file
 *   This file includes the board-specific configuration.
 *
 */

#ifndef TERBIUM_BOARD_CONFIG_H_
#define TERBIUM_BOARD_CONFIG_H_

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN
 *
 * Radio Coex output request pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN
#define TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN 14
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
 *
 * Radio Coex output priority pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
#define TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN 15
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
 *
 * Radio Coex output grant pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
#define TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN 20
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_TIMER
 *
 * Radio Coex timer.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_TIMER
#define TERBIUM_BOARD_CONFIG_COEX_TIMER 2
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN
 *
 * @note This pin is used when feature TERBIUM_BOARD_CONFIG_COEX_UNIT_TEST_ENABLE is enabled.
 *
 * Radio Coex Arbiter input request pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN
#define TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_REQUEST_PIN 18
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN
 *
 * @note This pin is used when feature TERBIUM_BOARD_CONFIG_COEX_UNIT_TEST_ENABLE is enabled.
 *
 * Radio Coex Arbiter input priotity pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN
#define TERBIUM_BOARD_CONFIG_COEX_ARBITER_IN_PRIORITY_PIN 19
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN
 *
 * @note This pin is used when feature TERBIUM_BOARD_CONFIG_COEX_UNIT_TEST_ENABLE is enabled.
 *
 * Radio Coex Arbiter output grant pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN
#define TERBIUM_BOARD_CONFIG_COEX_ARBITER_OUT_GRANT_PIN 16
#endif

#endif // TERBIUM_BOARD_CONFIG_H_
