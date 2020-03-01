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
 *   This file includes the terbium-specific configuration.
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
#define TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN 38 // Pin 1.7
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
 *
 * Radio Coex output priority pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
#define TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN 7 // Pin 0.7
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
 *
 * Radio Coex output grant pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
#define TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN 33 // Pin 1.1
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
 * @def TERBIUM_BOARD_CONFIG_FEM_CTX_PIN
 *
 * This setting configures the FEM transmit enable pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CTX_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CTX_PIN 36 // Pin 1.4
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CRX_PIN
 *
 * This setting configures the FEM receive enable pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CRX_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CRX_PIN 35 // Pin 1.3
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CPS_PIN
 *
 * This setting configures the FEM bypass TX/RX pin.
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CPS_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CPS_PIN 34 // Pin 1.2
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CHL_PIN
 *
 * This setting configures the FEM High/Low power pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CHL_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CHL_PIN 37 // Pin 1.5
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN
 *
 * This setting configures the volgate regulator enable pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN
#define TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN 40 // Pin 1.8
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ADJUST_PIN
 *
 * This setting configures the voltage regulator adjust pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ADJUST_PIN
#define TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ADJUST_PIN 43 // Pin 1.11
#endif
#endif // TERBIUM_BOARD_CONFIG_H_
