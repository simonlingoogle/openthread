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
 *   This file implements the hardware abstraction layer for FEM chip Sky66403.
 *
 */
#include <terbium-board-config.h>
#include <terbium-config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <utils/code_utils.h>

#include "chips/include/fem.h"
#include "chips/include/gpio.h"
#include "chips/include/radio.h"
#include "chips/peripherals/voltage_regulator.h"

#if TERBIUM_CONFIG_FEM_SKY66403_ENABLE
/*
 * SKY66403 supported FEM Modes:
 * +------+--------------+-------------+-------------------------------+------------------+
 * | MODE |      CPS     |    CHL      |     Transmit Mode             | Receive Mode     |
 * +------+--------------+-------------+-------------------------------+------------------+
 * |  0   |   Deactived  |  Deactived  | Transmit high-efficiency mode | Receive LNA mode |
 * +------+--------------+-------------+-------------------------------+------------------+
 * |  1   |   Deactived  |  Actived    | Transmit linear mode          | Receive LNA mode |
 * +------+--------------+-------------+-------------------------------+------------------+
 * |  2   |   Actived    |  Deactived  | Transmit bypass mode          | Receive LNA mode |
 * +------+--------------+-------------+-------------------------------+------------------+
 */

#define SKY66403_FEM_MODE_MIN 0x00
#define SKY66403_FEM_MODE_MAX 0x02
#define SKY66403_DEFAULT_FEM_MODE 0
#define SKY66403_FEM_LNA_GAIN_DBM 11

#define SKY66403_FEM_CHL_MASK (1U << 0)
#define SKY66403_FEM_CPS_MASK (1U << 1)

static uint8_t sFemMode = SKY66403_DEFAULT_FEM_MODE;
static bool    sFemInitilaized;

uint8_t tbHalFemGetDefaultMode(void)
{
    return SKY66403_DEFAULT_FEM_MODE;
}

bool tbHalFemIsModeValid(uint8_t aFemMode)
{
    return (aFemMode <= SKY66403_FEM_MODE_MAX) ? true : false;
}

otError tbHalFemGetModeRange(uint8_t *aMinMode, uint8_t *aMaxMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinMode != NULL) && (aMaxMode != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinMode = SKY66403_FEM_MODE_MIN;
    *aMaxMode = SKY66403_FEM_MODE_MAX;

exit:
    return error;
}

int8_t tbHalFemGetLnaGain(void)
{
    return SKY66403_FEM_LNA_GAIN_DBM;
}

static otError sky66403SetFemMode(uint8_t aFemMode)
{
    otError error;

    error = tbHalRadioFemPinControllerActivateModePin(TB_HAL_RADIO_FEM_MODE_PIN_CHL,
                                                      (aFemMode & SKY66403_FEM_CHL_MASK) ? true : false);
    otEXPECT(error == OT_ERROR_NONE);

    error = tbHalRadioFemPinControllerActivateModePin(TB_HAL_RADIO_FEM_MODE_PIN_CPS,
                                                      (aFemMode & SKY66403_FEM_CPS_MASK) ? true : false);

exit:
    return error;
}

otError tbHalFemInit(void)
{
    otError error = OT_ERROR_NONE;

    tbHalRadioFemPinControllerConfig femControllerConfig = {
        .mCtxPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CTX_PIN},
        .mCrxPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CRX_PIN},
        .mCsdPinConfig = {.mEnable = false, .mActiveHigh = false, .mGpioPin = 0xff},
        .mCpsPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CPS_PIN},
        .mChlPinConfig = {.mEnable = true, .mActiveHigh = true, .mGpioPin = TERBIUM_BOARD_CONFIG_FEM_CHL_PIN}};

    otEXPECT_ACTION(!sFemInitilaized, error = OT_ERROR_ALREADY);

    // Initialize voltage regulator.
    tbVoltageRegulatorInit();

    // Initialize radio FEM controller.
    tbHalRadioFemPinControllerInit(&femControllerConfig);

    sFemInitilaized = true;

exit:
    return error;
}

void tbHalFemDeinit(void)
{
    otEXPECT(sFemInitilaized);

    // Deinitialize voltage regulator.
    tbVoltageRegulatorDeinit();

    sFemInitilaized = false;
    sFemMode        = SKY66403_DEFAULT_FEM_MODE;

    sky66403SetFemMode(SKY66403_DEFAULT_FEM_MODE);

exit:
    return;
}

otError tbHalFemEnable(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(sFemInitilaized, error = OT_ERROR_FAILED);

    // Enable voltage regulator.
    tbVoltageRegulatorEnable();

    // Set FEM mode.
    error = sky66403SetFemMode(sFemMode);

exit:
    return error;
}

otError tbHalFemDisable(void)
{
    // Disable voltage regulator.
    tbVoltageRegulatorDisable();

    return sky66403SetFemMode(SKY66403_DEFAULT_FEM_MODE);
}

otError tbHalFemSetMode(uint8_t aFemMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(tbHalFemIsModeValid(aFemMode), error = OT_ERROR_INVALID_ARGS);
    otEXPECT(sFemMode != aFemMode);
    otEXPECT((error = sky66403SetFemMode(aFemMode)) == OT_ERROR_NONE);

    sFemMode = aFemMode;

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
#endif // TERBIUM_CONFIG_FEM_SKY66403_ENABLE
