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
 *   This file implements interfaces for wireless calibration.
 *
 */

#include <terbium-config.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>

#include "platform/sysenv.h"
#include "platform/sysenv_keys.h"
#include "platform/wireless_cal.h"

#if TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE

#define WIRELESSCAL_KEY_SIZE 32
#define WIRELESSCAL_CHANNEL_MIN 11
#define WIRELESSCAL_CHANNEL_MAX 26

static tbWirelessCalCalibration sCalibration = {TB_SYSENV_WIRELESS_CAL_IDENTIFIER, NULL};

static bool wirelessCalIsFrequencyInRange(uint32_t aFrequencyStart, uint32_t aFrequencyEnd, uint32_t aFrequency)
{
    return (aFrequency >= aFrequencyStart) && (aFrequency <= aFrequencyEnd);
}

bool tbWirelessCalIsChannelValid(uint8_t aChannel)
{
    return ((aChannel >= WIRELESSCAL_CHANNEL_MIN) && (aChannel <= WIRELESSCAL_CHANNEL_MAX)) ? true : false;
}

bool tbWirelessCalPowerSettingIsValid(const tbWirelessCalPowerSetting *aPowerSetting)
{
    return aPowerSetting->mCalibratedPower != 0;
}

bool tbWirelessCalSubbandTargetIsValid(const tbWirelessCalSubbandTarget *aSubbandTarget)
{
    return (aSubbandTarget->mFrequencyStart != 0) && (aSubbandTarget->mTargetPower != 0);
}

bool tbWirelessCalTargetIsValid(const tbWirelessCalTarget *aTarget)
{
    return (aTarget->mRegulatoryCode != 0) && (tbWirelessCalSubbandTargetIsValid(&aTarget->mSubbandTargets[0]));
}

bool tbWirelessCalSubbandSettingIsValid(const tbWirelessCalSubbandSetting *aSubbandSetting)
{
    return (aSubbandSetting->mFrequencyStart != 0) && (aSubbandSetting->mNumPowerSettings != 0) &&
           (tbWirelessCalPowerSettingIsValid(&aSubbandSetting->mPowerSettings[0]));
}

uint32_t tbWirelessCalMapFrequency(uint8_t aChannel)
{
    return (2400 + ((aChannel - 10) * 5));
}

uint8_t tbWirelessCalMapChannel(uint32_t aFrequency)
{
    uint8_t channel = (aFrequency - 2400) / 5 + 10;
    return tbWirelessCalIsChannelValid(channel) ? channel : 0;
}

const tbWirelessCalCalibration *tbWirelessCalGetCalibration(void)
{
    return &sCalibration;
}

static void loadWirelessCalParameterFromSysenv(void)
{
    char     key[WIRELESSCAL_KEY_SIZE];
    uint32_t size;

    snprintf(key, sizeof(key), "%s.%s", TB_SYSENV_KEY_WIRELESS_CAL, TB_SYSENV_WIRELESS_CAL_IDENTIFIER);
    otLogDebgPlat("%s: Loading From Sysenv, Key=%s", __func__, key);

    if (tbSysenvGetPointer(key, (const void **)&sCalibration.mParameters, &size) != OT_ERROR_NONE)
    {
        // Sysenv has not been set.
        sCalibration.mParameters = NULL;

        otLogNotePlat("%s: No Wireless Calibration table in sysenv", __func__);
    }
}

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static void sysenvNotifierCallback(void *aContext)
{
    OT_UNUSED_VARIABLE(aContext);

    // The address of the calibration table may has changed, reload calibration table from sysenv.
    loadWirelessCalParameterFromSysenv();
}
#endif

void tbWirelessCalInit(void)
{
#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
    static tbSysenvNotifier sSysenvNotifier = {sysenvNotifierCallback, NULL, NULL};

    tbSysenvNotifierRegister(&sSysenvNotifier);
#endif

    loadWirelessCalParameterFromSysenv();
}

static otError wirelessCalLookupSubbandSettings(const tbWirelessCalSubbandSetting *aSubBandSetting,
                                                uint32_t                           aFrequency,
                                                int32_t                            aTargetPower,
                                                int16_t *                          aCalibratedPower,
                                                int32_t *                          aEncodedPower)
{
    otError error = OT_ERROR_NOT_FOUND;

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; i++)
    {
        if (!tbWirelessCalSubbandSettingIsValid(aSubBandSetting))
        {
            // Hit the end.
            break;
        }

        if (wirelessCalIsFrequencyInRange(aSubBandSetting->mFrequencyStart, aSubBandSetting->mFrequencyEnd, aFrequency))
        {
            // Found the aSubBandSetting to use. The first entry should be the max
            // calibrated power with each subsequent entry a lower power value.
            // Use the first value less than or equal to the target power.
            const tbWirelessCalPowerSetting *powerSetting     = aSubBandSetting->mPowerSettings;
            const tbWirelessCalPowerSetting *prevPowerSetting = NULL;

            for (uint8_t j = 0; j < aSubBandSetting->mNumPowerSettings; j++, powerSetting++)
            {
                if (!tbWirelessCalPowerSettingIsValid(powerSetting))
                {
                    // Break out of search for the setting within the aSubBandSetting.
                    break;
                }

                prevPowerSetting = powerSetting;

                if (powerSetting->mCalibratedPower <= aTargetPower)
                {
                    // Found the calibrated power setting.
                    error = OT_ERROR_NONE;

                    if (aCalibratedPower)
                    {
                        *aCalibratedPower = powerSetting->mCalibratedPower;
                    }

                    if (aEncodedPower)
                    {
                        *aEncodedPower = powerSetting->mEncodedPower;
                    }

                    // Break out of search for the setting within the aSubBandSetting.
                    break;
                }
            }

            if (error == OT_ERROR_NOT_FOUND)
            {
                // We've either hit the end or the target power is lower than the lowest
                // power settings in the table, use the lowest power setting.
                if (prevPowerSetting)
                {
                    error = OT_ERROR_NONE;

                    if (aCalibratedPower)
                    {
                        *aCalibratedPower = prevPowerSetting->mCalibratedPower;
                    }

                    if (aEncodedPower)
                    {
                        *aEncodedPower = prevPowerSetting->mEncodedPower;
                    }
                }
            }

            // Break out of search for the aSubBandSetting.
            break;
        }

        // Move to next SubBandSetting entry.
        aSubBandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT_CONST(aSubBandSetting);
    }

    return error;
}

otError tbWirelessCalLookupTargetPower(uint16_t aRegulatoryCode, uint32_t aFrequency, int16_t *aTargetPower)
{
    otError                       error      = OT_ERROR_NOT_FOUND;
    const tbWirelessCalParameter *parameters = sCalibration.mParameters;
    const tbWirelessCalTarget *   target;

    otLogDebgPlat("%s: RegulatoryCode=0x%04x, Frequency=%d", __func__, aRegulatoryCode, aFrequency);

    VerifyOrExit(parameters != NULL);

    target = parameters->mTargetPowers;

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN; target++, i++)
    {
        if (!tbWirelessCalTargetIsValid(target))
        {
            // Hit the end.
            ExitNow();
        }

        if (target->mRegulatoryCode == aRegulatoryCode)
        {
            // Found the matching target regulatory code.

            const tbWirelessCalSubbandTarget *subbandTarget = target->mSubbandTargets;

            for (size_t j = 0; j < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; subbandTarget++, j++)
            {
                if (!tbWirelessCalSubbandTargetIsValid(subbandTarget))
                {
                    // Hit the end.
                    ExitNow();
                }

                if (wirelessCalIsFrequencyInRange(subbandTarget->mFrequencyStart, subbandTarget->mFrequencyEnd,
                                                  aFrequency))
                {
                    // Found it. Use this target power to lookup into the subband table.
                    *aTargetPower = subbandTarget->mTargetPower;
                    error         = OT_ERROR_NONE;

                    otLogDebgPlat("%s: Found TargetPower: %d", __func__, subbandTarget->mTargetPower);
                    ExitNow();
                }
            }

            ExitNow();
        }
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: Couldn't find TargetPower", __func__);
    }
    return error;
}

otError tbWirelessCalLookupPowerSetting(uint32_t aFrequency,
                                        int16_t  aTargetPower,
                                        int16_t *aCalibratedPower,
                                        int32_t *aEncodedPower)
{
    otError error = OT_ERROR_NOT_FOUND;

    otLogDebgPlat("%s: Frequency=%d, TargetPower=%d", __func__, aFrequency, aTargetPower);

    VerifyOrExit(sCalibration.mParameters != NULL);

    // Lookup the subband settings table.
    error = wirelessCalLookupSubbandSettings(sCalibration.mParameters->mSubbandSettings, aFrequency, aTargetPower,
                                             aCalibratedPower, aEncodedPower);

exit:
    if (error == OT_ERROR_NONE)
    {
        enum
        {
            kPowerStringSize = 10,
        };
        char calibratedPowerString[kPowerStringSize] = "NULL";
        char encodedPowerString[kPowerStringSize]    = "NULL";

        if (aCalibratedPower != NULL)
        {
            snprintf(calibratedPowerString, sizeof(calibratedPowerString), "%d", *aCalibratedPower);
        }

        if (aEncodedPower != NULL)
        {
            snprintf(encodedPowerString, sizeof(encodedPowerString), "%ld", *aEncodedPower);
        }

        otLogDebgPlat("%s: Found PowerSettings: CalibratedPower=%s, EncodedPower =%s", __func__, calibratedPowerString,
                      encodedPowerString);
    }
    else
    {
        otLogWarnPlat("%s: Couldn't find PowerSettings", __func__);
    }

    return error;
}

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
otError tbWirelessCalSave(const tbWirelessCalCalibration *aCalibration, uint32_t aBytesToWrite)
{
    otError error;
    char    key[WIRELESSCAL_KEY_SIZE];

    VerifyOrExit((aCalibration != NULL) && (aCalibration->mParameters != NULL) && (aBytesToWrite > 0),
                 error = OT_ERROR_INVALID_ARGS);

    snprintf(key, sizeof(key), "%s.%s", TB_SYSENV_KEY_WIRELESS_CAL, aCalibration->mIdentifier);

    otLogDebgPlat("%s: Key=%s, BytesToWrite=%d", __func__, key, aBytesToWrite);

    if ((error = tbSysenvSet(key, aCalibration->mParameters, aBytesToWrite)) != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: Write sysenv failed", __func__);
        ExitNow();
    }

exit:
    return error;
}
#endif // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
#endif // TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
