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
 *   This file implements the hardware abstraction layer for FEM chip Sky66408.
 *
 */

#include <terbium-board-config.h>
#include <terbium-config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <utils/code_utils.h>

#include "chips/include/delay.h"
#include "chips/include/fem.h"
#include "chips/include/gpio.h"
#include "chips/include/i2c.h"
#include "chips/include/radio.h"

#if TERBIUM_CONFIG_FEM_SKY66408_ENABLE
/*
 * SKY66408 supported FEM Modes:
 * +------+-------------+-----------------+--------------------------+
 * | MODE |    CHL      |  Transmit Mode  |        Receive Mode      |
 * +------+-------------+-----------------+--------------------------+
 * |  0   |  Deactived  |   Low Icc TX    | Low noise figure receive |
 * +------+-------------+-----------------+--------------------------+
 * |  1   |  Actived    |   High Icc TX   | Low noise figure receive |
 * +------+-------------+-----------------+--------------------------+
 */

#define SKY66408_I2C_ADDRESS 0x1B
#define SKY66408_I2C_HW_VERSION_REG_ADDRESS 0x00
#define SKY66408_I2C_POWER_INDEX_REG_ADDRESS 0x01
#define SKY66408_I2C_GPIO_STATUS_REG_ADDRESS 0x02
#define SKY66408_I2C_GPIO_CFG_REG_ADDRESS 0x03
#define SKY66408_I2C_GPIO_CTRL_REG_ADDRESS 0x04

#define SKY66408_LNA_GAIN_DBM 15
#define SKY66408_DEFAULT_FEM_MODE 0
#define SKY66408_MODE_MIN 0x00
#define SKY66408_MODE_MAX 0x01

#define SKY66408_DEFAULT_FEM_TXPOWER_REG 0
#define SKY66408_POWER_INDEX_MIN 0x00
#define SKY66408_POWER_INDEX_MAX 0x1F

static const tbHalI2cConfig sI2cConfig = {
    .mPinSda              = TERBIUM_BOARD_CONFIG_FEM_I2C_SDA_PIN,
    .mPinScl              = TERBIUM_BOARD_CONFIG_FEM_I2C_SCL_PIN,
    .mFrequency           = TB_HAL_I2C_FREQ_400K,
    .mRegisterAddressSize = TB_HAL_I2C_REGISTER_ADDRESS_SIZE_1_BYTE,
    .mSlaveAddressSize    = TB_HAL_I2C_SLAVE_ADDRESS_SIZE_7_BITS,
    .mSlaveAddress        = SKY66408_I2C_ADDRESS,
};

static uint8_t sFemPower = SKY66408_DEFAULT_FEM_TXPOWER_REG;
static uint8_t sFemMode  = SKY66408_DEFAULT_FEM_MODE;
static bool    sFemInitilaized;

bool tbHalFemIsPowerValid(uint8_t aFemPower)
{
    return (aFemPower <= SKY66408_POWER_INDEX_MAX) ? true : false;
}

otError tbHalFemGetPowerRange(uint8_t *aMinPower, uint8_t *aMaxPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinPower != NULL) && (aMaxPower != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinPower = SKY66408_POWER_INDEX_MIN;
    *aMaxPower = SKY66408_POWER_INDEX_MAX;

exit:
    return error;
}

bool tbHalFemIsModeValid(uint8_t aFemMode)
{
    return (aFemMode <= SKY66408_MODE_MAX);
}

otError tbHalFemGetModeRange(uint8_t *aMinMode, uint8_t *aMaxMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinMode != NULL) && (aMaxMode != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinMode = SKY66408_MODE_MIN;
    *aMaxMode = SKY66408_MODE_MAX;

exit:
    return error;
}

uint8_t tbHalFemGetDefaultMode(void)
{
    return SKY66408_DEFAULT_FEM_MODE;
}

uint8_t tbHalFemGetDefaultPower(void)
{
    return SKY66408_DEFAULT_FEM_TXPOWER_REG;
}

int8_t tbHalFemGetLnaGain(void)
{
    return SKY66408_LNA_GAIN_DBM;
}

tbHalFemPowerType tbHalFemGetPowerType(void)
{
    return TB_HAL_FEM_POWER_TYPE_REGISTER;
}

otError tbHalFemInit(void)
{
    otError error       = OT_ERROR_NONE;
    uint8_t femPowerReg = SKY66408_DEFAULT_FEM_TXPOWER_REG;

    tbHalRadioFemPinControllerConfig femControllerConfig = {
        .mCtxPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CTX_PIN},
        .mCrxPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CRX_PIN},
        .mCsdPinConfig = {.mEnable = false, .mActiveHigh = false, .mGpioPin = 0xff},
        .mCpsPinConfig = {.mEnable = false, .mActiveHigh = false, .mGpioPin = 0xff},
        .mChlPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CHL_PIN}};

    otEXPECT_ACTION(!sFemInitilaized, error = OT_ERROR_ALREADY);

    // Initialize I2C.
    otEXPECT((error = tbHalI2cInit(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, &sI2cConfig)) == OT_ERROR_NONE);

    // Initialize radio FEM controller.
    tbHalRadioFemPinControllerInit(&femControllerConfig);

    // Toggle the FEM reset pin.
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_FEM_RESET_PIN, TB_HAL_GPIO_VALUE_HIGH);
    tbHalDelayUs(2);
    tbHalGpioSetValue(TERBIUM_BOARD_CONFIG_FEM_RESET_PIN, TB_HAL_GPIO_VALUE_LOW);
    tbHalDelayUs(2);
    tbHalGpioSetValue(TERBIUM_BOARD_CONFIG_FEM_RESET_PIN, TB_HAL_GPIO_VALUE_HIGH);
    tbHalDelayUs(2);

    // Write default fem power.
    error = tbHalI2cWrite(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, SKY66408_I2C_POWER_INDEX_REG_ADDRESS, &femPowerReg,
                          sizeof(femPowerReg));
    otEXPECT(error == OT_ERROR_NONE);
    sFemInitilaized = true;

exit:
    return error;
}

void tbHalFemDeinit(void)
{
    otEXPECT(sFemInitilaized);

    tbHalGpioRelease(TERBIUM_BOARD_CONFIG_FEM_RESET_PIN);
    tbHalI2cDeinit(TERBIUM_BOARD_CONFIG_FEM_I2C_ID);

    sFemInitilaized = false;
    sFemPower       = SKY66408_DEFAULT_FEM_TXPOWER_REG;
    sFemMode        = SKY66408_DEFAULT_FEM_MODE;

exit:
    return;
}

otError tbHalFemEnable(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sFemInitilaized, error = OT_ERROR_FAILED);

    // Recovery the fem power.
    error = tbHalI2cWrite(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, SKY66408_I2C_POWER_INDEX_REG_ADDRESS, &sFemPower,
                          sizeof(sFemPower));

    otEXPECT(error == OT_ERROR_NONE);

    // FEM takes 800ns to hit 90% of target, delay 2us.
    tbHalDelayUs(2);

    // Activate FEM mode pin.
    error = tbHalRadioFemPinControllerActivateModePin(TB_HAL_RADIO_FEM_MODE_PIN_CHL, sFemMode ? true : false);

exit:
    return error;
}

otError tbHalFemDisable(void)
{
    otError error;
    uint8_t femPowerReg = SKY66408_DEFAULT_FEM_TXPOWER_REG;

    // Write default fem power.
    error = tbHalI2cWrite(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, SKY66408_I2C_POWER_INDEX_REG_ADDRESS, &femPowerReg,
                          sizeof(femPowerReg));
    otEXPECT(error == OT_ERROR_NONE);

    // FEM takes 800ns to hit 90% of target, delay 2us.
    tbHalDelayUs(2);

    // Deactivate FEM mode pin.
    error = tbHalRadioFemPinControllerActivateModePin(TB_HAL_RADIO_FEM_MODE_PIN_CHL, false);

exit:
    return error;
}

otError tbHalFemSetPower(uint8_t aFemPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aFemPower <= SKY66408_POWER_INDEX_MAX, error = OT_ERROR_INVALID_ARGS);

    otEXPECT(sFemPower != aFemPower);

    // Write FEM power register.
    error = tbHalI2cWrite(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, SKY66408_I2C_POWER_INDEX_REG_ADDRESS, &aFemPower,
                          sizeof(aFemPower));
    otEXPECT(error == OT_ERROR_NONE);

    // FEM takes 800ns to hit 90% of target, delay 2us.
    tbHalDelayUs(2);

    sFemPower = aFemPower;

exit:
    return error;
}

otError tbHalFemGetPower(uint8_t *aFemPower)
{
    otError error;

    otEXPECT_ACTION(aFemPower != NULL, error = OT_ERROR_INVALID_ARGS);
    error =
        tbHalI2cRead(TERBIUM_BOARD_CONFIG_FEM_I2C_ID, SKY66408_I2C_POWER_INDEX_REG_ADDRESS, aFemPower, sizeof(uint8_t));

exit:
    return error;
}

otError tbHalFemSetMode(uint8_t aFemMode)
{
    otError error = OT_ERROR_NONE;
    bool    activate;

    otEXPECT_ACTION(tbHalFemIsModeValid(aFemMode), error = OT_ERROR_INVALID_ARGS);
    otEXPECT(sFemMode != aFemMode);

    sFemMode = aFemMode;
    activate = aFemMode ? true : false;

    error = tbHalRadioFemPinControllerActivateModePin(TB_HAL_RADIO_FEM_MODE_PIN_CHL, activate);

exit:
    return error;
}

otError tbHalFemGetMode(uint8_t *aFemMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aFemMode != NULL, error = OT_ERROR_INVALID_ARGS);

    *aFemMode = sFemMode;

exit:
    return error;
}
#endif // TERBIUM_CONFIG_FEM_SKY66408_ENABLE
