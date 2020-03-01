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
 *   This file includes the hardware abstraction layer for FEM.
 *
 */

#ifndef CHIPS_INCLUDE_FEM_H__
#define CHIPS_INCLUDE_FEM_H__

#include <stdint.h>
#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration defines constants that are used to indicate different FEM power types.
 *
 */
typedef enum
{
    TB_HAL_FEM_POWER_TYPE_VCC2,     ///< The FEM power is VCC2 value.
    TB_HAL_FEM_POWER_TYPE_REGISTER, ///< The FEM power is FEM register value.
} tbHalFemPowerType;

/**
 * This function initialize the GPIOs and peripheral bus for FEM control.
 *
 * @retval OT_ERROR_NONE     Successfully initialized the FEM.
 * @retval OT_ERROR_ALREADY  FEM was already initialized.
 * @retval OT_ERROR_FAILED   Failed to enable the FEM.
 *
 */
otError tbHalFemInit(void);

/**
 * This function deinitializes the GPIOs or peripheral bus for FEM control, put FEM in lowest power state.
 *
 */
void tbHalFemDeinit(void);

/**
 * This function indicates whether or not the FEM power is supported by FEM.
 *
 * @param[in]  aFemPower  FEM power.
 *
 * @retval true   FEM power is supported.
 * @retval false  FEM power is not supported.
 *
 */
bool tbHalFemIsPowerValid(uint8_t aFemPower);

/**
 * This function gets FEM power range.
 *
 * @param[out]  aMinPower  The minimum FEM power.
 * @param[out]  aMaxPower  The maximum FEM power.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved FEM power range.
 * @retval OT_ERROR_INVALID_ARGS  The @p aMinPower or @p aMaxPower was NULL.
 *
 */
otError tbHalFemGetPowerRange(uint8_t *aMinPower, uint8_t *aMaxPower);

/**
 * This function indicates whether or not the FEM mode is supported by FEM.
 *
 * @param[in]  aFemMode  FEM mode.
 *
 * @retval true   FEM mode is supported.
 * @retval false  FEM mode is not supported.
 *
 */
bool tbHalFemIsModeValid(uint8_t aFemMode);

/**
 * This function gets FEM mode range.
 *
 * @param[out]  aMinMode  The minimum FEM mode value.
 * @param[out]  aMaxMode  The maximum FEM mode value.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved FEM mode range.
 * @retval OT_ERROR_INVALID_ARGS  The @p aMinMode or @p aMaxMode was NULL.
 *
 */
otError tbHalFemGetModeRange(uint8_t *aMinMode, uint8_t *aMaxMode);

/**
 * This function returns the default FEM mode.
 *
 * @returns The default FEM mode.
 *
 */
uint8_t tbHalFemGetDefaultMode(void);

/**
 * This function returns the default FEM power.
 *
 * @returns The default FEM power.
 *
 */
uint8_t tbHalFemGetDefaultPower(void);

/**
 * This function enables FEM and put it in RX state.
 *
 * @note This function should be called before the radio leaving SLEEP state.
 *
 * @retval OT_ERROR_NONE    Successfully enabled FEM module.
 * @retval OT_ERROR_FAILED  Failed to enable FEM module.
 *
 */
otError tbHalFemEnable(void);

/**
 * This function disables FEM and put it in lowest power state.
 *
 * @note This function should be called before the radio entering SLEEP state.
 *
 * @retval OT_ERROR_NONE    Successfully disabled FEM module.
 * @retval OT_ERROR_FAILED  Failed to disable FEM module.
 *
 */
otError tbHalFemDisable(void);

/**
 * This function returns FEM LNA gain.
 *
 * @returns The default FEM LNA gain, in dBm.
 *
 */
int8_t tbHalFemGetLnaGain(void);

/**
 * This function returns current FEM power type.
 *
 * @retval TB_HAL_FEM_POWER_TYPE_VCC2      The FEM power is VCC2 value
 * @retval TB_HAL_FEM_POWER_TYPE_REGISTER  The FEM power is FEM register value.
 *
 */
tbHalFemPowerType tbHalFemGetPowerType(void);

/**
 * This function sets FEM power.
 *
 * @note The value of FEM power can be FEM vcc2 or FEM register value, the
 *       specific meaning is determined by the FEM module.
 *
 * @param[in]  aFemPower  FEM power.
 *
 * @retval OT_ERROR_NONE          Successfully set FEM power.
 * @retval OT_ERROR_INVALID_ARGS  The @p aFemPower was not supported by FEM.
 *
 */
otError tbHalFemSetPower(uint8_t aFemPower);

/**
 * This function gets current used FEM power.
 *
 * @note The value of FEM power can be FEM vcc2 or FEM register value, the
 *       specific meaning is determined by the FEM module. User calls
 *       `tbHalFemGetPowerType()` to get the specific meaning of the power.
 *
 * @param[out]  aFemPower  FEM power.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved FEM power.
 * @retval OT_ERROR_FAILED        Read FEM power failed.
 * @retval OT_ERROR_INVALID_ARGS  The @p aFemPower was NULL.
 *
 */
otError tbHalFemGetPower(uint8_t *aFemPower);

/**
 * This function sets FEM mode.
 *
 * @param[in]  aFemMode  FEM mode.
 *
 * @retval OT_ERROR_NONE          Successfully set FEM power.
 * @retval OT_ERROR_INVALID_ARGS  The @p aFemMode was not supported by FEM.
 *
 */
otError tbHalFemSetMode(uint8_t aFemMode);

/**
 * This function gets current used FEM mode.
 *
 * @param[out]  aFemMode  FEM mode.
 *
 * @retval OT_ERROR_NONE          Successfully retrieved FEM mode.
 * @retval OT_ERROR_INVALID_ARGS  The @p aFemMode was NULL.
 *
 */
otError tbHalFemGetMode(uint8_t *aFemMode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_FEM_H__
