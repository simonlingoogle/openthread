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
#define TERBIUM_BOARD_CONFIG_COEX_OUT_REQUEST_PIN 10
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
 *
 * Radio Coex output priority pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN
#define TERBIUM_BOARD_CONFIG_COEX_OUT_PRIORITY_PIN 8
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
 *
 * Radio Coex output grant pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN
#define TERBIUM_BOARD_CONFIG_COEX_IN_GRANT_PIN 9
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
 * @def TERBIUM_BOARD_CONFIG_FEM_I2C_ID
 *
 * This setting configures the FEM I2C bus identifier.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_I2C_ID
#define TERBIUM_BOARD_CONFIG_FEM_I2C_ID 0
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_I2C_SDA_PIN
 *
 * This setting configures the FEM I2C bus SDA pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_I2C_SDA_PIN
#define TERBIUM_BOARD_CONFIG_FEM_I2C_SDA_PIN 25
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_I2C_SCL_PIN
 *
 * This setting configures the FEM I2C bus SCL pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_I2C_SCL_PIN
#define TERBIUM_BOARD_CONFIG_FEM_I2C_SCL_PIN 26
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CTX_PIN
 *
 * This setting configures the FEM transmit enable pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CTX_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CTX_PIN 19
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CRX_PIN
 *
 * This setting configures the FEM receive enable pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CRX_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CRX_PIN 18
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_RESET_PIN
 *
 * This setting configures the FEM reset pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_RESET_PIN
#define TERBIUM_BOARD_CONFIG_FEM_RESET_PIN 28
#endif

/**
 * @def TERBIUM_BOARD_CONFIG_FEM_CHL_PIN
 *
 * This setting configures the FEM High/Low power pin.
 *
 */
#ifndef TERBIUM_BOARD_CONFIG_FEM_CHL_PIN
#define TERBIUM_BOARD_CONFIG_FEM_CHL_PIN 29
#endif

#endif // TERBIUM_BOARD_CONFIG_H_
