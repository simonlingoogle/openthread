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
 *   This file includes Terbium compile-time configuration constants.
 *
 */

#ifndef TERBIUM_CONFIG_H__
#define TERBIUM_CONFIG_H__

/**
 * @def TERBIUM_CONFIG_FEM_SKY66408_ENABLE
 *
 * Define to 1 to enable FEM sky66408 support.
 *
 */
#ifndef TERBIUM_CONFIG_FEM_SKY66408_ENABLE
#define TERBIUM_CONFIG_FEM_SKY66408_ENABLE 1
#endif

/**
 * @def TERBIUM_CONFIG_RADIO_ANTENNA_CCA_THRESHOLD
 *
 * The CCA threshold on antenna.
 *
 */
#ifndef TERBIUM_CONFIG_RADIO_ANTENNA_CCA_THRESHOLD
#define TERBIUM_CONFIG_RADIO_ANTENNA_CCA_THRESHOLD -74
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
 * @def TERBIUM_CONFIG_NO_WIRELESS_CALIBRATION_POWER_SETTINGS_TABLE
 *
 * @note This configuration is used when feature TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE is disabled.
 *
 * Some products have very accurate output power and do not require power calibration.
 * This macro defines the power settings table that does not need to be calibrated.
 *
 * The actual value defined by this macro of comes from b/152263012.
 *
 */
#define TERBIUM_CONFIG_NO_WIRELESS_CALIBRATION_POWER_SETTINGS_TABLE                                                    \
    {                                                                                                                  \
        {TB_REGULATORY_DOMAIN_AGENCY_ETSI, 11, 26, {0, 13, 1}}, {TB_REGULATORY_DOMAIN_AGENCY_FCC, 11, 24, {0, 31, 1}}, \
            {TB_REGULATORY_DOMAIN_AGENCY_FCC, 25, 25, {0, 29, 1}},                                                     \
    }

/**
 * @def TERBIUM_CONFIG_BACKTRACE_ENABLE
 *
 * Define to 1 to enable backtrace support.
 *
 */
#ifndef TERBIUM_CONFIG_BACKTRACE_ENABLE
#define TERBIUM_CONFIG_BACKTRACE_ENABLE 1
#endif

#if defined(TERBIUM_BUILD_ENG)
/**
 * @def TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
 *
 * Define to 1 to enable sysenv write support.
 *
 */
#ifndef TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
#define TERBIUM_CONFIG_SYSENV_WRITE_ENABLE 1
#endif
#endif // defined(TERBIUM_BUILD_ENG)

#include "chips/peripherals/config.h"
#include "platform/config.h"
#endif // TERBIUM_CONFIG_H__
