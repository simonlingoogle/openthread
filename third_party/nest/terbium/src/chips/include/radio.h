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
 *   This file includes the hardware abstraction layer for radio.
 *
 */

#ifndef CHIPS_INCLUDE_RADIO_H__
#define CHIPS_INCLUDE_RADIO_H__

#include <stdint.h>
#include "chips/include/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This enumeration defines the FEM mode pin identifiers.
 *
 */
typedef enum
{
    TB_HAL_RADIO_FEM_MODE_PIN_CHL = 0, ///< High/Low power pin identifier.
    TB_HAL_RADIO_FEM_MODE_PIN_CPS = 1, ///< Bypass TX/RX pin identifier.
} tbHalRadioFemModePin;

/**
 * This structure defines the configuration parameters of FEM pin.
 *
 */
typedef struct tbHalRadioFemPinConfig
{
    bool mEnable;          ///< Enable toggling for this pin.
    bool mActiveHigh;      ///< If true, the pin will be active high.
                           ///< Otherwise, the pin will be active low.
    tbHalGpioPin mGpioPin; ///< GPIO pin number for the pin.
} tbHalRadioFemPinConfig;

/**
 * This structure defines the configuration parameters of FEM pin controller.
 *
 */
typedef struct tbHalRadioFemPinControllerConfig
{
    tbHalRadioFemPinConfig mCtxPinConfig; ///< Transmit enable pin configuration.
    tbHalRadioFemPinConfig mCrxPinConfig; ///< Receive enable pin configuration.
    tbHalRadioFemPinConfig mCsdPinConfig; ///< Power shutdown pin configuration.
    tbHalRadioFemPinConfig mCpsPinConfig; ///< Bypass TX/RX pin configuration.
    tbHalRadioFemPinConfig mChlPinConfig; ///< High/Low power pin configuration.
} tbHalRadioFemPinControllerConfig;

/**
 * This function indicates whether or not the radio power is supported by the radio driver.
 *
 * @param[in]  aRadioPower Radio power in unit of 1 dBm.
 *
 * @retval true   The radio power is supported.
 * @retval false  The radio power is not supported.
 *
 */
bool tbHalRadioIsPowerValid(int8_t aRadioPower);

/**
 * This function returns the default radio power.
 *
 * @param[in]  aRadioPower Radio power in unit of 1 dBm.
 *
 * @returns The default radio power.
 *
 */
int8_t tbHalRadioGetDefaultPower(void);

/**
 * This function returns the radio supported power list.
 *
 * @param[out]  aNumPower  Number of powers in the power list.
 *
 * @returns The pointer pointed to the power list. If @p aNumPowers is NULL, returns NULL.
 *
 */
const int8_t *tbHalRadioGetSupportedPowerList(uint8_t *aNumPowers);

/**
 * This function returns the current used channel.
 *
 * @returns The current used channel.
 *
 */
uint8_t tbHalRadioGetChannel(void);

/**
 * This function changes the radio state to emit continuous carrier.
 *
 * @retval OT_ERROR_NONE    Successfully emitted continuous carrier.
 * @retval OT_ERROR_FAILED  The driver could not schedule the continuous carrier procedure.
 *
 */
otError tbHalRadioEmitContinuousCarrier(void);

/**
 * This function sets the FEM mode, FEM power and radio power. These power settings take
 * affect after user calls the function 'tbHalRadioUseFactoryPowerSettings()'.
 *
 * @param[in]  aFemMode    FEM mode.
 * @param[in]  aFemPower   FEM power setting.
 * @param[in]  aRadioPower Radio power in unit of 1 dBm.
 *
 * @retval OT_ERROR_NONE          Successfully set power settings.
 * @retval OT_ERROR_INVALID_ARGS  The power settings are not supported by radio driver or FEM.
 *
 */
otError tbHalRadioSetFactoryPowerSettings(uint8_t aFemMode, uint8_t aFemPower, int8_t aRadioPower);

/**
 * This function validates the power settings set by the function 'tbHalRadioSetFactoryPowerSettings()'.
 */
void tbHalRadioUseFactoryPowerSettings(void);

/**
 * This function invalidates the power settings set by the function 'tbHalRadioSetFactoryPowerSettings()'.
 */
void tbHalRadioResetFactoryPowerSettings(void);

/**
 * This function sets the regulatory code and target power. These two parameters
 * are used by radio driver to lookup the wireless calibration table and power
 * settings table to find the appropriate FEM mode, FEM power and radio power.
 * The radio driver uses the found power settings to set transmit power.
 *
 * @note If power settings can not be found based on these two parameters, radio
 *       driver uses default power settings. If user has called
 *       'tbHalRadioUseFactoryPowerSettings()', radio driver directly uses the
 *       power settings set by 'tbHalRadioSetFactoryPowerSettings()' to set the
 *       transmit power, it won't lookup the wireless calibration table.
 *
 * @param[in]  aRegulatoryCode  Regulatory domain code.
 * @param[in]  aTargetPower     Target power in unit of 0.01 dBm.
 *
 */
void tbHalRadioSetRegulatoryTargetPower(uint16_t aRegulatoryCode, int16_t aTargetPower);

/**
 * This function initializes the radio FEM pin controller.
 *
 * @param[in]  aConfig   A pointer to the configuration parameters of FEM pin controller.
 *
 */
void tbHalRadioFemPinControllerInit(tbHalRadioFemPinControllerConfig *aConfig);

/**
 * This function activates or inactivates the given FEM mode pin.
 *
 * Note: If FEM mode pin is activated, the pin's behavior is consistent with the
 *       behavior of CTX pin. If FEM mode pin is inactivated, the pin is set to
 *       inactive state.
 *
 * @param[in]  aModePin   FEM mode pin.
 * @param[in]  aActivate  Indicate whether the given pin should be activated or inactivated.
 *
 * @retval OT_ERROR_NONE    The FEM mode pin was successfully configured.
 * @retval OT_ERROR_FAILED  Failed to ativate the FEM mode pin.
 *
 */
otError tbHalRadioFemPinControllerActivateModePin(tbHalRadioFemModePin aModePin, bool aActivate);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_RADIO_H__
