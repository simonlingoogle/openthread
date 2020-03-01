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
 *   This file implements the hardware abstraction layer for FEM simlation.
 *
 */

#include <terbium-board-config.h>
#include <terbium-config.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <utils/code_utils.h>

#include "chips/include/fem.h"

#if TERBIUM_CONFIG_FEM_SIM_ENABLE

#define FEM_DEFAULT_MODE 0
#define FEM_MODE_MIN 0x00
#define FEM_MODE_MAX 0x02
#define FEM_MODE_INVALID 0xFF

#define FEM_DEFAULT_POWER 0
#define FEM_POWER_MIN 0x00
#define FEM_POWER_MAX 0xFE
#define FEM_POWER_INVALID 0xFF

static bool    sFemInitilaized;
static uint8_t sFemPower;
static uint8_t sFemMode;

bool tbHalFemIsPowerValid(uint8_t aFemPower)
{
    return (aFemPower <= FEM_POWER_MAX) ? true : false;
}

otError tbHalFemGetPowerRange(uint8_t *aMinPower, uint8_t *aMaxPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinPower != NULL) && (aMaxPower != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinPower = FEM_POWER_MIN;
    *aMaxPower = FEM_POWER_MAX;

exit:
    return error;
}

bool tbHalFemIsModeValid(uint8_t aFemMode)
{
    return (aFemMode <= FEM_MODE_MAX) ? true : false;
}

int8_t tbHalFemGetLnaGain(void)
{
    return 0;
}

otError tbHalFemGetModeRange(uint8_t *aMinMode, uint8_t *aMaxMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinMode != NULL) && (aMaxMode != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinMode = FEM_MODE_MIN;
    *aMaxMode = FEM_MODE_MAX;

exit:
    return error;
}

uint8_t tbHalFemGetDefaultMode(void)
{
    return FEM_DEFAULT_MODE;
}

uint8_t tbHalFemGetDefaultPower(void)
{
    return FEM_DEFAULT_POWER;
}

tbHalFemPowerType tbHalFemGetPowerType(void)
{
    return TB_HAL_FEM_POWER_TYPE_REGISTER;
}

otError tbHalFemInit(void)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(!sFemInitilaized, error = OT_ERROR_ALREADY);

    sFemInitilaized = true;
    sFemPower       = FEM_POWER_INVALID;
    sFemMode        = FEM_POWER_INVALID;

exit:
    return error;
}

void tbHalFemDeinit(void)
{
    otEXPECT(sFemInitilaized);

    sFemInitilaized = false;
    sFemPower       = FEM_POWER_INVALID;
    sFemMode        = FEM_MODE_INVALID;

exit:
    return;
}

otError tbHalFemEnable(void)
{
    return OT_ERROR_NONE;
}

otError tbHalFemDisable(void)
{
    return OT_ERROR_NONE;
}

otError tbHalFemSetPower(uint8_t aFemPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aFemPower <= FEM_POWER_MAX, error = OT_ERROR_INVALID_ARGS);

    sFemPower = aFemPower;

exit:
    return error;
}

otError tbHalFemGetPower(uint8_t *aFemPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aFemPower != NULL, error = OT_ERROR_INVALID_ARGS);
    *aFemPower = sFemPower;

exit:
    return error;
}

otError tbHalFemSetMode(uint8_t aFemMode)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(tbHalFemIsModeValid(aFemMode), error = OT_ERROR_INVALID_ARGS);

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
#endif // TERBIUM_CONFIG_FEM_SIM_ENABLE
