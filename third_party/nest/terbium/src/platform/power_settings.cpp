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
 *   This file implements interfaces for power settings.
 *
 */

#include <terbium-config.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <openthread/platform/toolchain.h>

#include "chips/include/fem.h"
#include "chips/include/radio.h"
#include "platform/power_settings.h"
#include "platform/sysenv.h"
#include "platform/sysenv_keys.h"
#include "platform/wireless_cal.h"

#if TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE

#define POWER_SETTINGS_KEY_SIZE 32
#define POWER_SETTING_INVALID_VALUE 0x7f

/**
 * This structure represents the power setting entry.
 */
OT_TOOL_PACKED_BEGIN
struct _PowerSettingEntry
{
    uint8_t        mId;            ///< Identifier used for lookup.
    tbPowerSetting mPowerSettings; ///< Power setting structure.
} OT_TOOL_PACKED_END;

typedef struct _PowerSettingEntry PowerSettingEntry;

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static PowerSettingEntry sRamPowerSettingsTable[TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV];
static uint8_t           sNumPowerSettingsEntries;
#endif

static const PowerSettingEntry *sFlashPowerSettingsTable;
static const PowerSettingEntry *sFlashPowerSettingsTableEnd;

static inline bool isPowerSettingEmpty(const tbPowerSetting *aSetting)
{
    return ((aSetting->mFemMode == POWER_SETTING_INVALID_VALUE) &&
            (aSetting->mFemPower == POWER_SETTING_INVALID_VALUE) &&
            (aSetting->mRadioPower == POWER_SETTING_INVALID_VALUE))
               ? true
               : false;
}

static bool isPowerSettingsValid(const tbPowerSetting *aSetting)
{
    bool ret = true;

    if (!tbHalRadioIsPowerValid(aSetting->mRadioPower))
    {
        otLogWarnPlat("%s: %d is not a valid radio power in dBm value", __func__, aSetting->mRadioPower);
        ExitNow(ret = false);
    }

    if (!tbHalFemIsModeValid(aSetting->mFemMode))
    {
        otLogWarnPlat("%s: %d is not a valid fem mode", __func__, aSetting->mFemMode);
        ExitNow(ret = false);
    }

    if (!tbHalFemIsPowerValid(aSetting->mFemPower))
    {
        otLogWarnPlat("%s: %d is not a valid fem power", __func__, aSetting->mFemPower);
        ExitNow(ret = false);
    }

exit:
    return ret;
}

static void loadPowerSettingsFromSysenv(void)
{
    char     key[POWER_SETTINGS_KEY_SIZE];
    uint32_t size;

    snprintf(key, sizeof(key), "%s.%s", TB_SYSENV_KEY_WIRELESS_CAL, TB_SYSENV_POWER_SETTINGS_IDENTIFIER);
    otLogDebgPlat("%s: Loading From Sysenv, Key=%s", __func__, key);

    if (tbSysenvGetPointer(key, (const void **)&sFlashPowerSettingsTable, &size) == OT_ERROR_NONE)
    {
        // Found power settings table from sysenv.
        sFlashPowerSettingsTableEnd = (const PowerSettingEntry *)((const uint8_t *)sFlashPowerSettingsTable + size);
    }
    else
    {
        // Sysenv has not been set.
        sFlashPowerSettingsTable    = NULL;
        sFlashPowerSettingsTableEnd = NULL;

        otLogNotePlat("%s: No Power Settings table in sysenv", __func__);
    }
}

static otError powerSettingsGetNext(uint16_t *aIterater, const PowerSettingEntry **aEntry)
{
    otError                  error = OT_ERROR_NONE;
    const PowerSettingEntry *entry;

    VerifyOrExit((aIterater != NULL) && (aEntry != NULL), error = OT_ERROR_INVALID_ARGS);
    VerifyOrExit(sFlashPowerSettingsTable != NULL, error = OT_ERROR_NOT_FOUND);

    entry = &sFlashPowerSettingsTable[*aIterater];
    VerifyOrExit((entry < sFlashPowerSettingsTableEnd) && !isPowerSettingEmpty(&entry->mPowerSettings),
                 error = OT_ERROR_NOT_FOUND);

    *aEntry    = entry;
    *aIterater = *aIterater + 1;

exit:
    return error;
}

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static void sysenvNotifierCallback(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    // The address of the power setting table may has changed, reload the power settings table from sysenv.
    loadPowerSettingsFromSysenv();
}
#endif

void tbPowerSettingsInit(void)
{
#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
    static tbSysenvNotifier sSysenvNotifier = {sysenvNotifierCallback, NULL, NULL};

    tbSysenvNotifierRegister(&sSysenvNotifier);

    sNumPowerSettingsEntries = 0;
#endif

    loadPowerSettingsFromSysenv();
}

otError tbPowerSettingsDecode(int32_t aId, tbPowerSetting *aPowerSetting)
{
    otError                  error    = OT_ERROR_NOT_FOUND;
    uint16_t                 iterater = TB_POWER_SETTINGS_ITERATOR_INIT;
    const PowerSettingEntry *entry    = NULL;

    VerifyOrExit(aPowerSetting != NULL, error = OT_ERROR_INVALID_ARGS);

    while (powerSettingsGetNext(&iterater, &entry) == OT_ERROR_NONE)
    {
        if (entry->mId == aId)
        {
            // Found a match.
            if (!isPowerSettingsValid(&entry->mPowerSettings))
            {
                otLogNotePlat("%s: Tx power settings for identifier(%u) exists, but not valid", __func__, aId);
                ExitNow(error = OT_ERROR_NOT_FOUND);
            }

            ExitNow(error = OT_ERROR_NONE);
        }
    }

exit:
    if (error == OT_ERROR_NONE)
    {
        *aPowerSetting = entry->mPowerSettings;
    }

    return error;
}

otError tbPowerSettingsGetNext(uint16_t *aIterater, int32_t *aId, const tbPowerSetting **aPowerSetting)
{
    const PowerSettingEntry *entry;
    otError                  error = powerSettingsGetNext(aIterater, &entry);

    if (error == OT_ERROR_NONE)
    {
        *aId           = entry->mId;
        *aPowerSetting = &entry->mPowerSettings;
    }

    return error;
}

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static inline bool powerSettingCompare(const tbPowerSetting *aSettingA, const tbPowerSetting *aSettingB)
{
    return ((aSettingA->mFemMode == aSettingB->mFemMode) && (aSettingA->mFemPower == aSettingB->mFemPower) &&
            (aSettingA->mRadioPower == aSettingB->mRadioPower))
               ? true
               : false;
}

otError tbPowerSettingsEncode(const tbPowerSetting *aPowerSetting, int32_t *aId)
{
    otError  error       = OT_ERROR_NONE;
    uint32_t index       = 0;
    uint8_t  updateTable = 0;

    VerifyOrExit((aPowerSetting != NULL) && isPowerSettingsValid(aPowerSetting) && (aId != NULL),
                 error = OT_ERROR_INVALID_ARGS);

    if (sNumPowerSettingsEntries == 0)
    {
        memset(sRamPowerSettingsTable, POWER_SETTING_INVALID_VALUE, sizeof(sRamPowerSettingsTable));
        updateTable = 1;
    }
    else
    {
        // Iterate over the existing table in RAM to see if the entry exists, if not create one.
        for (index = 0; index < TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV; index++)
        {
            if (isPowerSettingEmpty(&sRamPowerSettingsTable[index].mPowerSettings))
            {
                // Found an empty slot first meaning the input setting doesn't exist.
                updateTable = 1;
                break;
            }
            else if (powerSettingCompare(&sRamPowerSettingsTable[index].mPowerSettings, aPowerSetting))
            {
                // Found a matching entry in the table.
                *aId = sRamPowerSettingsTable[index].mId;
                ExitNow();
            }
        }
    }

    // The last entry is used as an end marker.
    VerifyOrExit(index < TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV - 1, error = OT_ERROR_NO_BUFS);

    if (updateTable)
    {
        *aId = index;

        sRamPowerSettingsTable[index].mId            = index;
        sRamPowerSettingsTable[index].mPowerSettings = *aPowerSetting;

        otLogDebgPlat("%s: Update power table, PowerSettingIndex=%d, RadioPower=%d, FemMode=%d, FemPower=%d", __func__,
                      sNumPowerSettingsEntries, aPowerSetting->mRadioPower, aPowerSetting->mFemMode,
                      aPowerSetting->mFemPower);

        sNumPowerSettingsEntries++;
    }

exit:
    return error;
}

otError tbPowerSettingsSave(void)
{
    otError  error;
    char     key[POWER_SETTINGS_KEY_SIZE];
    uint32_t powerTableSize;

    VerifyOrExit(sNumPowerSettingsEntries != 0, error = OT_ERROR_FAILED);

    // Add an entry as an end marker.
    sNumPowerSettingsEntries++;

    VerifyOrExit(sNumPowerSettingsEntries <= TERBIUM_CONFIG_NUM_POWER_SETTINGS_IN_SYSENV, error = OT_ERROR_FAILED);

    powerTableSize           = sNumPowerSettingsEntries * sizeof(sRamPowerSettingsTable[0]);
    sNumPowerSettingsEntries = 0;

    snprintf(key, sizeof(key), "%s.%s", TB_SYSENV_KEY_WIRELESS_CAL, TB_SYSENV_POWER_SETTINGS_IDENTIFIER);

    otLogDebgPlat("%s: Writting to sysenv for key %s", __func__, key);

    // Save the temporary power table to sysenv.
    VerifyOrExit((error = tbSysenvSet(key, &sRamPowerSettingsTable[0], powerTableSize)) == OT_ERROR_NONE, OT_NOOP);

exit:
    return error;
}
#else  // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
otError tbPowerSettingsEncode(const tbPowerSetting *aPowerSetting, int32_t *aId)
{
    OT_UNUSED_VARIABLE(aPowerSetting);
    OT_UNUSED_VARIABLE(aId);

    return OT_ERROR_NOT_IMPLEMENTED;
}

otError tbPowerSettingsSave(void)
{
    return OT_ERROR_NOT_IMPLEMENTED;
}
#endif // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
#endif // TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
