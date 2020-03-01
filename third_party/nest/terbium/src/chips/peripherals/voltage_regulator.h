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
 *   This file defines the voltage regulator API.
 *
 */
#ifndef CHIPS_PERIPHERALS_VOLTAGE_REGULATOR_H__
#define CHIPS_PERIPHERALS_VOLTAGE_REGULATOR_H__

#include <stdint.h>

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function initializes the voltage regulator.
 *
 * @retval OT_ERROR_NONE    Initialization was successful.
 * @retval OT_ERROR_FAILED  Failed to initialize the voltage regulator.
 *
 */
otError tbVoltageRegulatorInit(void);

/**
 * This function deinitializes the voltage regulator.
 *
 */
void tbVoltageRegulatorDeinit(void);

/**
 * This function enables the voltage regulator.
 *
 * @retval OT_ERROR_NONE    Successfully enabled the voltage regulator.
 * @retval OT_ERROR_FAILED  Failed to enable the voltage regulator.
 *
 */
otError tbVoltageRegulatorEnable(void);

/**
 * This function disables the voltage regulator.
 *
 * @retval OT_ERROR_NONE    Successfully disabled the voltage regulator.
 * @retval OT_ERROR_FAILED  Failed to disable the voltage regulator.
 *
 */
otError tbVoltageRegulatorDisable(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_PERIPHERALS_VOLTAGE_REGULATOR_H__
