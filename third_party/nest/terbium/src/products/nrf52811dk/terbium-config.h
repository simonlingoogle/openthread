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
 * @def TERBIUM_CONFIG_FEM_SIM_ENABLE
 *
 * Define to 1 to enable FEM simulation support.
 *
 */
#ifndef TERBIUM_CONFIG_FEM_SIM_ENABLE
#define TERBIUM_CONFIG_FEM_SIM_ENABLE 1
#endif

/**
 * @def TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
 *
 * Define to 1 to enable wireless calibration support.
 *
 */
#ifndef TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
#define TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE 1
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
