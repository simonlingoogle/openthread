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
 *   This file implements the power manager.
 *
 */

#include <openthread-core-config.h>

#include <stdio.h>

#include <common/code_utils.hpp>
#include <openthread/platform/toolchain.h>

#include "platform/power_manager.h"
#include "platform/regulatory_domain.h"
#include "platform/wireless_cal.h"

#if TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
static otError lookupEncodedPower(uint16_t aRegulatoryCode,
                                  uint8_t  aChannel,
                                  int16_t  aTargetPower,
                                  int32_t *aEncodedPower)
{
    otError  error = OT_ERROR_NONE;
    uint32_t frequency;
    int16_t  targetPower;

    frequency = tbWirelessCalMapFrequency(aChannel);

    if (aTargetPower == TB_WIRELESSCAL_INVALID_TARGET_POWER)
    {
        // If target power is invalid, try to lookup the default target power as the target power.
        error = tbWirelessCalLookupTargetPower(aRegulatoryCode, frequency, &targetPower);
        VerifyOrExit(error == OT_ERROR_NONE);
    }
    else
    {
        targetPower = aTargetPower;
    }

    // Lookup dencoded power from wireless calibration table.
    error = tbWirelessCalLookupPowerSetting(frequency, targetPower, NULL, aEncodedPower);

exit:
    return error;
}

otError tbPowerManagerGetPowerSetting(uint16_t        aRegulatoryCode,
                                      uint8_t         aChannel,
                                      int16_t         aTargetPower,
                                      tbPowerSetting *aPowerSetting)
{
    otError error = OT_ERROR_NOT_FOUND;
    int32_t encodedPower;

    VerifyOrExit((aRegulatoryCode != TB_REGULATORY_DOMAIN_UNKNOWN) && (aPowerSetting != NULL),
                 error = OT_ERROR_INVALID_ARGS);

    // Lookup encoded power from wireless calibration table.
    if (lookupEncodedPower(aRegulatoryCode, aChannel, aTargetPower, &encodedPower) == OT_ERROR_NONE)
    {
        // Use decoded power as the key to lookup power settings from power settings table.
        if (tbPowerSettingsDecode(encodedPower, aPowerSetting) == OT_ERROR_NONE)
        {
            error = OT_ERROR_NONE;
        }
    }

exit:
    return error;
}

#else // TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE

#if !defined(TERBIUM_CONFIG_NO_WIRELESS_CALIBRATION_POWER_SETTINGS_TABLE)
#error "No TERBIUM_CONFIG_NO_WIRELESS_CALIBRATION_POWER_SETTINGS_TABLE was defined."
#endif

typedef struct RegulatoryPowerSetting
{
    uint16_t       mRegulatoryCode;
    uint8_t        mChannelStart;
    uint8_t        mChannelEnd;
    tbPowerSetting mPowerSetting;
} RegulatoryPowerSetting;

const static RegulatoryPowerSetting sNoCalibrationPowerSettingsTable[] =
    TERBIUM_CONFIG_NO_WIRELESS_CALIBRATION_POWER_SETTINGS_TABLE;

otError tbPowerManagerGetPowerSetting(uint16_t        aRegulatoryCode,
                                      uint8_t         aChannel,
                                      int16_t         aTargetPower,
                                      tbPowerSetting *aPowerSetting)
{
    otError                       error    = OT_ERROR_NOT_FOUND;
    const RegulatoryPowerSetting *settings = sNoCalibrationPowerSettingsTable;
    uint8_t                       i;

    OT_UNUSED_VARIABLE(aTargetPower);

    VerifyOrExit(aPowerSetting != NULL, error = OT_ERROR_INVALID_ARGS);

    for (i = 0; i < OT_ARRAY_LENGTH(sNoCalibrationPowerSettingsTable); i++)
    {
        if ((settings[i].mRegulatoryCode == aRegulatoryCode) && (settings[i].mChannelStart <= aChannel) &&
            (aChannel <= settings[i].mChannelEnd))
        {
            *aPowerSetting = settings[i].mPowerSetting;
            error          = OT_ERROR_NONE;
            break;
        }
    }

exit:
    return error;
}
#endif // TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
