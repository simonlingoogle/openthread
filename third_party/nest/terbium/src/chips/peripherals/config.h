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
 *   This file includes peripherals compile-time configuration constants.
 *
 */

#ifndef PERIPHERALS_CONFIG_H__
#define PERIPHERALS_CONFIG_H__

/**
 * @def TERBIUM_CONFIG_FEM_SIM_ENABLE
 *
 * Define to 1 to enable FEM simulation support.
 *
 */
#ifndef TERBIUM_CONFIG_FEM_SIM_ENABLE
#define TERBIUM_CONFIG_FEM_SIM_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_FEM_SKY66408_ENABLE
 *
 * Define to 1 to enable FEM sky66408 support.
 *
 */
#ifndef TERBIUM_CONFIG_FEM_SKY66408_ENABLE
#define TERBIUM_CONFIG_FEM_SKY66408_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_FEM_SKY66403_ENABLE
 *
 * Define to 1 to enable FEM sky66403 support.
 *
 */
#ifndef TERBIUM_CONFIG_FEM_SKY66403_ENABLE
#define TERBIUM_CONFIG_FEM_SKY66403_ENABLE 0
#endif

/**
 * @def TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
 *
 * Define to 1 to enable volgate regulator TLV75801 support.
 *
 */
#ifndef TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
#define TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE 0
#endif

#endif // PERIPHERALS_CONFIG_H__
