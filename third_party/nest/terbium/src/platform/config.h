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
 *   This file includes platform compile-time configuration constants.
 *
 */

#ifndef PLATFORM_CONFIG_H__
#define PLATFORM_CONFIG_H__

/**
 * @def TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE
 *
 * This setting configures the max RAM buffer size in sysenv.
 *
 */
#ifndef TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE
#define TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE 700
#endif

/**
 * @def TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
 *
 * Define to 1 to enable sysenv write support.
 *
 */
#ifndef TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
#define TERBIUM_CONFIG_SYSENV_WRITE_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
 *
 * Define to 1 to enable wireless calibration support.
 *
 */
#ifndef TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
#define TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV
 *
 * This setting configures the maximum number of radio power settings in sysenv.
 *
 */
#ifndef TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV
#define TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV 64
#endif

/**
 * @def TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN
 *
 * This setting configures the maximum number of regulatory domains in wireless calibration table.
 *
 */
#ifndef TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN
#define TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN 2
#endif

/**
 * @def TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND
 *
 * This setting configures the maximum number of wireless calibration subbands.
 *
 */
#ifndef TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND
#define TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND 16
#endif

/**
 * @def TERBIUM_CONFIG_BACKTRACE_ENABLE
 *
 * Define to 1 to enable backtrace support.
 *
 */
#ifndef TERBIUM_CONFIG_BACKTRACE_ENABLE
#define TERBIUM_CONFIG_BACKTRACE_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
 *
 * Define to 1 to enable radio coex unit test module.
 *
 */
#ifndef TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE
#define TERBIUM_CONFIG_COEX_UNIT_TEST_ENABLE 0
#endif

#endif // PLATFORM_CONFIG_H__
