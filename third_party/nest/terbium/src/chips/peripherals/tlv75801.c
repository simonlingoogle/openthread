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
 *   This file implements the functions for low-dropout regulator chip TLV75801.
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
#include "chips/peripherals/voltage_regulator.h"

/**
 * @def TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
 *
 * Define to 1 to enable voltage regulator TLV75801 support.
 *
 */
#ifndef TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
#define TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE 0
#endif

#if TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
/*
 * EncodedVoltage output table (V):
 * +---------+-------+-------+-------+
 * | ADJ_PIN |  MIN  | NORM  | MAX   |
 * +---------+-------+-------+-------+
 * |   LOW   | 0.649 | 0.650 | 0.658 |
 * +---------+-------+-------+-------+
 * |  HIGH   | 2.893 | 2.967 | 3.043 |
 * +---------+-------+-------+-------+
 */

#define TLV75801_FEM_VCC2_MASK 0x01
#define TLV75801_FEM_VCC2_MIN 0
#define TLV75801_FEM_VCC2_MAX 1
#define TLV75801_DEFAULT_FEM_VCC2 0x01

static uint16_t sFemPower = TLV75801_DEFAULT_FEM_VCC2;

bool tbHalFemIsPowerValid(uint8_t aFemPower)
{
    return (aFemPower <= TLV75801_FEM_VCC2_MAX) ? true : false;
}

otError tbHalFemGetPowerRange(uint8_t *aMinPower, uint8_t *aMaxPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinPower != NULL) && (aMaxPower != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinPower = TLV75801_FEM_VCC2_MIN;
    *aMaxPower = TLV75801_FEM_VCC2_MAX;

exit:
    return error;
}

tbHalFemPowerType tbHalFemGetPowerType(void)
{
    return TB_HAL_FEM_POWER_TYPE_VCC2;
}

static void tlv75801SetEncodedVoltage(uint8_t aEncodedVoltage)
{
    uint8_t value = (aEncodedVoltage & TLV75801_FEM_VCC2_MASK) ? TB_HAL_GPIO_VALUE_HIGH : TB_HAL_GPIO_VALUE_LOW;

    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ADJUST_PIN, value);
}

otError tbVoltageRegulatorInit(void)
{
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN, TB_HAL_GPIO_VALUE_HIGH);
    tlv75801SetEncodedVoltage(TLV75801_DEFAULT_FEM_VCC2);

    return OT_ERROR_NONE;
}

void tbVoltageRegulatorDeinit(void)
{
    tlv75801SetEncodedVoltage(0);
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN, TB_HAL_GPIO_VALUE_LOW);
}

otError tbVoltageRegulatorEnable(void)
{
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN, TB_HAL_GPIO_VALUE_HIGH);
    tlv75801SetEncodedVoltage(sFemPower);

    return OT_ERROR_NONE;
}

otError tbVoltageRegulatorDisable(void)
{
    tlv75801SetEncodedVoltage(0);
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN, TB_HAL_GPIO_VALUE_LOW);

    return OT_ERROR_NONE;
}

otError tbHalFemSetPower(uint8_t aFemPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(tbHalFemIsPowerValid(aFemPower), error = OT_ERROR_FAILED);
    otEXPECT(sFemPower != aFemPower);

    sFemPower = aFemPower;
    tlv75801SetEncodedVoltage(aFemPower);

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

uint8_t tbHalFemGetDefaultPower(void)
{
    return TLV75801_DEFAULT_FEM_VCC2;
}

#endif // TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
