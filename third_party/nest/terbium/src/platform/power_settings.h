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

/**
 * @file
 *   This file includes definitions for processing power settings.
 */

#ifndef PLATFORM_POWER_SETTINGS_H__
#define PLATFORM_POWER_SETTINGS_H__

#include <stdint.h>
#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_POWER_SETTINGS_ITERATOR_INIT 0 ///< Initializer for power settings table iterator.

/**
 * This structure represents the power setting values.
 */
OT_TOOL_PACKED_BEGIN
struct _tbPowerSetting
{
    int8_t  mRadioPower; ///< Radio power in unit of 1 dBm.
    uint8_t mFemPower;   ///< FEM power setting, which represents vcc2 value or FEM power register value.
                         ///< The specific meaning is determined by the FEM module.
    uint8_t mFemMode;    ///< FEM mode.
} OT_TOOL_PACKED_END;

typedef struct _tbPowerSetting tbPowerSetting;

/**
 * This function initializes the power settings table.
 */
void tbPowerSettingsInit(void);

/**
 * This function gets power settings info at aIterator position.
 *
 * @param[inout] aIterator      A iterator to the index of the power setting entry.
 *                              To get the first power setting entry, it should be
 *                              set to TB_POWER_SETTINGS_ITERATOR_INIT.
 * @param[out]   aId            A pointer to the identifier.
 * @param[out]   aPowerSetting  A pointer to the power setting structure.
 *
 * @retval OT_ERROR_NONE          Successfully got the power setting info.
 * @retval OT_ERROR_INVALID_ARGS  The @p aIterator is NULL.
 * @retval OT_ERROR_NOT_FOUND     Not found the power setting entry.
 *
 */
otError tbPowerSettingsGetNext(uint16_t *aIterater, int32_t *aId, const tbPowerSetting **aPowerSetting);

/**
 * This function writes the power setting to inner temporary buffer and generates an
 * identifier used to lookup the power setting.
 *
 * @note This function is used when feature TERBIUM_CONFIG_FACTORY_DIAG_COMMAND_ENABLE
 *       is enabled. User shoud call 'tbPowerSettingsSave()' to write the temporary
 *       buffer to sysenv after all power settings are set.
 *
 * @param[in]  aPowerSetting  A pointer to power setting structure.
 * @param[out] aId            A pointer to an identifier used to lookup the power setting.
 *
 * @retval OT_ERROR_NONE          Successfully encoded the power setting.
 * @retval OT_ERROR_INVALID_ARGS  @p aPowerSetting or @p aId is NULL, or the power
 *                                setting are not supported by radio.
 * @retval OT_ERROR_NO_BUFS       Insufficient buffer space to store the power setting.
 */
otError tbPowerSettingsEncode(const tbPowerSetting *aPowerSetting, int32_t *aId);

/**
 * This function decodes the power setting from the power setting table in sysenv.
 *
 * @param[in]   aId            The identifier used to lookup the power setting.
 * @param[out]  aPowerSetting  A pointer to power setting structure.
 *
 * @retval OT_ERROR_NONE          Successfully retrived the power setting.
 * @retval OT_ERROR_INVALID_ARGS  @p aPowerSetting is NULL.
 * @retval OT_ERROR_NOT_FOUND     Not found the power setting for the given identifier.
 */
otError tbPowerSettingsDecode(int32_t aId, tbPowerSetting *aPowerSetting);

/**
 * This function writes all power setting in temporary buffer to sysenv.
 *
 * @note This function is used when feature TERBIUM_CONFIG_FACTORY_DIAG_COMMAND_ENABLE is enabled.
 *
 * @retval OT_ERROR_NONE    Successfully saved the power setting to sysenv.
 * @retval OT_ERROR_FAILED  Failed to write power setting to sysenv.
 */
otError tbPowerSettingsSave(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_POWER_SETTINGS_H__
