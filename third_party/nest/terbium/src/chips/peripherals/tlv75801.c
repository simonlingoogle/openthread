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

/*
 * On Newman, the FEM Power is composed of three bits. Each bit represents a control pin of the voltage regulator chip.
 * The chip tlv75801 has only one control pin. The bit 2 of the FEM Power indicates the voltage level of tlv75801's
 * control pin. For other two bits, the tlv75801 driver doesn't process them.
 *
 */
#define FEM_POWER_VCC2_PIN0_MASK (1U << 0)
#define FEM_POWER_VCC2_PIN1_MASK (1U << 1)
#define FEM_POWER_VCC2_PIN2_MASK (1U << 2)
#define FEM_POWER_VCC2_MIN 0x00
#define FEM_POWER_VCC2_MAX 0x07
#define FEM_POWER_VCC2_DEFAULT 0x07

static uint16_t sFemPower = FEM_POWER_VCC2_DEFAULT;

bool tbHalFemIsPowerValid(uint8_t aFemPower)
{
    return (aFemPower <= FEM_POWER_VCC2_MAX) ? true : false;
}

otError tbHalFemGetPowerRange(uint8_t *aMinPower, uint8_t *aMaxPower)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION((aMinPower != NULL) && (aMaxPower != NULL), error = OT_ERROR_INVALID_ARGS);

    *aMinPower = FEM_POWER_VCC2_MIN;
    *aMaxPower = FEM_POWER_VCC2_MAX;

exit:
    return error;
}

tbHalFemPowerType tbHalFemGetPowerType(void)
{
    return TB_HAL_FEM_POWER_TYPE_VCC2;
}

static void tlv75801SetEncodedVoltage(uint8_t aEncodedVoltage)
{
    uint8_t value = (aEncodedVoltage & FEM_POWER_VCC2_PIN2_MASK) ? TB_HAL_GPIO_VALUE_HIGH : TB_HAL_GPIO_VALUE_LOW;

    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ADJUST_PIN, value);
}

otError tbVoltageRegulatorInit(void)
{
    tbHalGpioOutputRequest(TERBIUM_BOARD_CONFIG_VOLTAGE_REGULATOR_ENABLE_PIN, TB_HAL_GPIO_VALUE_HIGH);
    tlv75801SetEncodedVoltage(FEM_POWER_VCC2_DEFAULT);

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
    return FEM_POWER_VCC2_DEFAULT;
}

#endif // TERBIUM_CONFIG_VOLTAGE_REGULATOR_TLV75801_ENABLE
