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
 *   This file implements Wireless Calibration diag commands.
 *
 */

#include <openthread-core-config.h>
#include <terbium-config.h>

#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/toolchain.h>

#include "chips/include/fem.h"
#include "chips/include/radio.h"
#include "platform/diag.h"
#include "platform/diag_wireless_cal.h"
#include "platform/power_settings.h"
#include "platform/regulatory_domain.h"
#include "platform/sysenv_keys.h"
#include "platform/wireless_cal.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE && TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
static const tbWirelessCalCalibration *wirelessCalGetCalibration(void)
{
    const tbWirelessCalCalibration *calibration = tbWirelessCalGetCalibration();

    if (!calibration)
    {
        tbDiagResponseOutput("Discrete calibration not found for identifier %s\n", TB_SYSENV_WIRELESS_CAL_IDENTIFIER);
    }

    return calibration;
}

static void printWirelessCalTarget(const tbWirelessCalTarget *aTarget, bool aShowSetSyntax)
{
    char                              codeString[TB_REGULATORY_DOMAIN_STRING_SIZE];
    const char *                      codeName;
    uint16_t                          startChannel;
    uint16_t                          numChannel;
    const tbWirelessCalSubbandTarget *subbandTarget;

    VerifyOrExit(aTarget != NULL, OT_NOOP);

    subbandTarget = aTarget->mSubbandTargets;

    tbRegulatoryDomainDecode(aTarget->mRegulatoryCode, codeString, sizeof(codeString));

    if (aShowSetSyntax)
    {
        tbDiagResponseOutput("  -r %s", codeString);
    }
    else
    {
        switch (aTarget->mRegulatoryCode)
        {
        case TB_REGULATORY_DOMAIN_AGENCY_IC:
        case TB_REGULATORY_DOMAIN_AGENCY_FCC:
            codeName = "FCC";
            break;

        case TB_REGULATORY_DOMAIN_AGENCY_ETSI:
            codeName = "ETSI";
            break;

        default:
            codeName = "UNKNOWN";
            break;
        }

        tbDiagResponseOutput("  %s %s: [", codeString, codeName);
    }

    for (size_t j = 0; j < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; j++, subbandTarget++)
    {
        if (!tbWirelessCalSubbandTargetIsValid(subbandTarget))
        {
            break;
        }

        if (aShowSetSyntax)
        {
            tbDiagResponseOutput("%s", (j == 0) ? " -t " : "/");
        }
        else
        {
            if (j > 0)
            {
                tbDiagResponseOutput(", ");
            }
        }

        startChannel = tbWirelessCalMapChannel(subbandTarget->mFrequencyStart);
        numChannel   = tbWirelessCalMapChannel(subbandTarget->mFrequencyEnd) - startChannel + 1;

        tbDiagResponseOutput("(%u,%u,%u)", startChannel, numChannel, subbandTarget->mTargetPower);
    }

    tbDiagResponseOutput(aShowSetSyntax ? "\n" : "]");

exit:
    return;
}

static void printWirelessCalPowerSetting(const tbWirelessCalPowerSetting *aPowerSetting, bool aSetFormat)
{
    int32_t        encodedPower = aPowerSetting->mEncodedPower;
    tbPowerSetting powerSetting;

    tbPowerSettingsDecode(encodedPower, &powerSetting);

    if (aSetFormat)
    {
        tbDiagResponseOutput("%d,%d,%u,%u", aPowerSetting->mCalibratedPower, powerSetting.mRadioPower,
                             powerSetting.mFemMode, powerSetting.mFemPower);
    }
    else
    {
        char femPowerIdentifier = (tbHalFemGetPowerType() == TB_HAL_FEM_POWER_TYPE_VCC2) ? 'v' : 'i';
        tbDiagResponseOutput("(%d,%ddBm,%u,%c%u)", aPowerSetting->mCalibratedPower, powerSetting.mRadioPower,
                             powerSetting.mFemMode, femPowerIdentifier, powerSetting.mFemPower);
    }
}

static void printWirelessCalSubbandSetting(const tbWirelessCalSubbandSetting *aSubbandSetting, bool aShowSyntax)
{
    const tbWirelessCalPowerSetting *powerSetting;
    uint16_t                         startChannel;
    uint16_t                         numChannel;

    VerifyOrExit(aSubbandSetting != NULL, OT_NOOP);

    powerSetting = aSubbandSetting->mPowerSettings;
    startChannel = tbWirelessCalMapChannel(aSubbandSetting->mFrequencyStart);
    numChannel   = tbWirelessCalMapChannel(aSubbandSetting->mFrequencyEnd) - startChannel + 1;

    if (aShowSyntax)
    {
        tbDiagResponseOutput("  -b %u,%u,%u", startChannel, numChannel, aSubbandSetting->mNumPowerSettings);
    }
    else
    {
        tbDiagResponseOutput("  (%u,%u,%u) [", startChannel, numChannel, aSubbandSetting->mNumPowerSettings);
    }

    for (size_t j = 0; j < aSubbandSetting->mNumPowerSettings; j++, powerSetting++)
    {
        if (!tbWirelessCalPowerSettingIsValid(powerSetting))
        {
            break;
        }

        if (aShowSyntax)
        {
            if (j == 0)
            {
                tbDiagResponseOutput(" -s ");
            }
            else
            {
                tbDiagResponseOutput("/");
            }
        }
        else
        {
            if (j > 0)
            {
                tbDiagResponseOutput(", ");
            }
        }

        printWirelessCalPowerSetting(powerSetting, false);
    }

    tbDiagResponseOutput(aShowSyntax ? "\n" : "]");

exit:
    return;
}

static void printWirelessCalParameter(const tbWirelessCalParameter *aParameter)
{
    const tbWirelessCalTarget *        target;
    const tbWirelessCalSubbandSetting *subbandSetting;

    // Output Example:
    // TargetPower:
    //   A1 ETSI: [(11,3,1000), (14,11,1100), (25,2,1000)]
    //   A2 FCC: [(11,3,1200), (14,11,1190), (25,2,1200)]
    // SubbandSettings:
    //   (11,3,4) [(1050,-4,0,7), (970,-4,0,3), (630,-8,0,7), (580,-8,0,3)]
    //   (14,11,4) [(1020,-4,1,6), (980,-4,1,3), (620,-8,0,6), (600,-8,0,2)]
    //   (25,2,4) [(1010,0,0,5), (950,0,0,0), (610,-4,0,2), (590,-4,0,0)]
    // In Set Syntax:
    //   -r A1 -t 11,3,1000/14,11,1100/25,2,1000
    //   -b 11,3,4 -s 1050,-4,0,7/970,-4,0,3/630,-8,0,7/580,-8,0,3
    //   -b 14,11,4 -s 1020,-4,1,6/980,-4,1,3/620,-8,0,6/600,-8,0,2
    //   -b 25,2,4 -s 1010,0,0,5/950,0,0,0/610,-4,0,2/590,-4,0,0

    VerifyOrExit(aParameter != NULL, OT_NOOP);

    target         = aParameter->mTargetPowers;
    subbandSetting = aParameter->mSubbandSettings;

    tbDiagResponseOutput("TargetPower:\n");

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN; i++, target++)
    {
        if (!tbWirelessCalTargetIsValid(target))
        {
            break;
        }

        if (i > 0)
        {
            tbDiagResponseOutput(",\n");
        }

        printWirelessCalTarget(target, false);
    }

    tbDiagResponseOutput("\nSubbandSettings:\n");

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; i++)
    {
        if (!tbWirelessCalSubbandSettingIsValid(subbandSetting))
        {
            break;
        }

        if (i > 0)
        {
            tbDiagResponseOutput(",\n");
        }

        printWirelessCalSubbandSetting(subbandSetting, false);

        // Move to next SubBandSetting entry.
        subbandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT_CONST(subbandSetting);
    }

    tbDiagResponseOutput("\nIn Set Syntax:\n");

    target         = aParameter->mTargetPowers;
    subbandSetting = aParameter->mSubbandSettings;

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN; i++, target++)
    {
        if (!tbWirelessCalTargetIsValid(target))
        {
            break;
        }

        printWirelessCalTarget(target, true);
    }

    for (size_t i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; i++)
    {
        if (!tbWirelessCalSubbandSettingIsValid(subbandSetting))
        {
            break;
        }

        printWirelessCalSubbandSetting(subbandSetting, true);

        // Move to next SubBandSetting entry.
        subbandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT_CONST(subbandSetting);
    }

    tbDiagResponseOutput("\n");

exit:
    return;
}

static otError printWirelessCalCalibration(void)
{
    otError                         error       = OT_ERROR_NONE;
    const tbWirelessCalCalibration *calibration = wirelessCalGetCalibration();

    VerifyOrExit(calibration != NULL, error = OT_ERROR_FAILED);

    if (calibration->mParameters)
    {
        tbDiagResponseOutput("Calibration(%s): Type %u\n", TB_SYSENV_WIRELESS_CAL_IDENTIFIER,
                             kWirelessCalTechnologyType6LoWPAN);
        printWirelessCalParameter(calibration->mParameters);
    }
    else
    {
        tbDiagResponseOutput("Calibration parameter is empty\n");
        error = OT_ERROR_FAILED;
    }

exit:
    return error;
}

static otError printPowerSettings(void)
{
    uint16_t              iterater = TB_POWER_SETTINGS_ITERATOR_INIT;
    int32_t               id;
    const tbPowerSetting *powerSetting;

    tbDiagResponseOutput("\nPower settings table in sysenv:\n");
    while (tbPowerSettingsGetNext(&iterater, &id, &powerSetting) == OT_ERROR_NONE)
    {
        tbDiagResponseOutput(" Id=%d RadioPower=%d, FemMode=%d, FemPower=%d\n", id, powerSetting->mRadioPower,
                             powerSetting->mFemMode, powerSetting->mFemPower);
    }
    tbDiagResponseOutput("End\n");

    return OT_ERROR_NONE;
}

static otError processWirelessCalGet(uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    VerifyOrExit((aArgsLength >= 1) && (strcmp(aArgs[0], "get") == 0), OT_NOOP);

    if (aArgsLength == 1)
    {
        error = printWirelessCalCalibration();
    }
    else if ((aArgsLength == 2) && (strcmp(aArgs[1], "powersettings") == 0))
    {
        error = printPowerSettings();
    }

exit:
    return error;
}

static uint8_t wirelessCalLookupTargetIndex(const tbWirelessCalParameter *aParameters, uint16_t aRegulatoryCode)
{
    uint8_t                    targetIndex;
    const tbWirelessCalTarget *target = aParameters->mTargetPowers;

    for (targetIndex = 0; targetIndex < TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN; targetIndex++, target++)
    {
        // Hit the end, return the index.
        if (!tbWirelessCalTargetIsValid(target))
        {
            break;
        }

        // Found a match.
        if (target->mRegulatoryCode == aRegulatoryCode)
        {
            break;
        }
    }

    return targetIndex;
}

static bool wirelessCalIsSubbandSettingValid(const tbWirelessCalSubbandSetting *subbandSetting)
{
    bool valid = true;

    // Check order of the powerSettings in previously completed subbandSetting.
    // The powerSettings should be sorted from high power to low power,
    // because that's what the wirelesscal library will expect when doing the lookup.
    for (size_t i = 1; i < subbandSetting->mNumPowerSettings; i++)
    {
        if (subbandSetting->mPowerSettings[i].mCalibratedPower > subbandSetting->mPowerSettings[i - 1].mCalibratedPower)
        {
            tbDiagResponseOutput(
                "\tError: SubbandSetting invalid, mCalibratedPower %d for index %u higher than %u for index %u\n",
                subbandSetting->mPowerSettings[i].mCalibratedPower, i,
                subbandSetting->mPowerSettings[i - 1].mCalibratedPower, i - 1);
            valid = false;
            break;
        }
        else if (subbandSetting->mPowerSettings[i].mCalibratedPower ==
                 subbandSetting->mPowerSettings[i - 1].mCalibratedPower)
        {
            tbDiagResponseOutput(
                "\tError: SubbandSetting invalid, mCalibratedPower %d for index %u equal to %u for index %u\n",
                subbandSetting->mPowerSettings[i].mCalibratedPower, i,
                subbandSetting->mPowerSettings[i - 1].mCalibratedPower, i - 1);
            valid = false;
            break;
        }
    }

    if (!valid)
    {
        tbDiagResponseOutput(
            "\tError: PowerSettings in SubbandSetting should be ordered from highest power to lowest power\n");
    }

    return valid;
}

static otError wirelessCalSetCalibration(const tbWirelessCalCalibration *aCalibration,
                                         uint8_t                         aArgsLength,
                                         char *                          aArgs[])
{
    enum
    {
        kStackBufferSize = 450,
    };
    otError                      error;
    uint8_t                      stackBuffer[kStackBufferSize] = {0};
    uint8_t *                    stackBufferEnd                = stackBuffer + kStackBufferSize;
    tbWirelessCalCalibration     calibration    = {aCalibration->mIdentifier, (tbWirelessCalParameter *)stackBuffer};
    tbWirelessCalParameter *     parameters     = (tbWirelessCalParameter *)stackBuffer;
    tbWirelessCalTarget *        target         = NULL;
    tbWirelessCalSubbandSetting *subbandSetting = NULL;
    uint16_t                     subbandSettingIndex = 0;
    int                          optionIndex         = 0;
    int                          ch;

    const struct option options[] = {
        {(char *)"r", 1, NULL, 'r'}, // Regulator domain.
        {(char *)"t", 1, NULL, 't'}, // Target power subband for the regulator domain.
        {(char *)"b", 1, NULL, 'b'}, // Calibrated subband setting info (ChannelStart, ChannelCount, NumPowerSettings).
        {(char *)"s", 1, NULL, 's'}, // PowerSettings for subbandSetting.
        {0, 0, 0, 0},
    };

    // Command example:
    // wirelesscal set -r A0 -t 11,3,1100/14,11,1000/25,2,1100 -r A1 -t 11,16,1000
    // -b 11,3,4 -s 1050,-4,1,30/970,-4,1,24/630,-8,1,31/580,-8,1,12
    // -b 14,11,4 -s 1020,-4,1,31/980,-4,1,26/620,-8,1,28/600,-8,1,22
    // -b 25,2,4 -s 1010,0,1,28/950,0,1,26/610,-4,1,30/590,-4,1,20

    optind = 0;
    while ((ch = getopt_long(aArgsLength, aArgs, "r:t:b:s:", options, &optionIndex)) != -1)
    {
        switch (ch)
        {
        case 'r':
        {
            uint16_t regulatoryCode = tbRegulatoryDomainEncode(optarg);
            uint16_t targetIndex    = wirelessCalLookupTargetIndex(parameters, regulatoryCode);

            otLogDebgPlat("Regulatory: RegCode=0x%x, targetIndex=%u", regulatoryCode, targetIndex);
            if (targetIndex < TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN)
            {
                target = &parameters->mTargetPowers[targetIndex];

                // Store the regulatory code for this power table.
                target->mRegulatoryCode = regulatoryCode;
            }
            else
            {
                tbDiagResponseOutput("\tError: Unable to find regulatoryCode %s in existing table, cannot use this cmd "
                                     "to create new regulatoryCode entry. Must modify in product defaults\n",
                                     optarg);
                target = NULL;
            }
        }
        break;

        case 't':
        {
            char * endptr = optarg;
            size_t i;

            if (target == NULL)
            {
                tbDiagResponseOutput("\tError: You need to specify a valid regulatory domain code first\n");
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            for (i = 0; i < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND; i++)
            {
                tbWirelessCalSubbandTarget *subbandTarget = &target->mSubbandTargets[i];

                uint8_t channelStart;
                uint8_t channelCount;
                uint8_t channelEnd;
                int16_t targetPower;

                channelStart = (uint8_t)strtol(endptr, &endptr, 10);
                VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

                channelCount = (uint8_t)strtol(endptr + 1, &endptr, 10);
                VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

                targetPower = (int16_t)strtol(endptr + 1, &endptr, 10);
                channelEnd  = channelStart + channelCount - 1;

                if ((!tbWirelessCalIsChannelValid(channelStart)) || (!tbWirelessCalIsChannelValid(channelEnd)))
                {
                    tbDiagResponseOutput("\tError: Channel %u or count %u are not valid\n", channelStart, channelCount);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }

                subbandTarget->mFrequencyStart = tbWirelessCalMapFrequency(channelStart);
                subbandTarget->mFrequencyEnd   = tbWirelessCalMapFrequency(channelEnd);
                subbandTarget->mTargetPower    = targetPower;

                otLogDebgPlat("subbandTarget: FreqStart=%d, FreqEnd=%d TargetPower=%d", subbandTarget->mFrequencyStart,
                              subbandTarget->mFrequencyEnd, subbandTarget->mTargetPower);

                if (*endptr == '/')
                {
                    // More entries coming.
                    endptr++;
                }
                else if (*endptr == '\0')
                {
                    break;
                }
                else
                {
                    tbDiagResponseOutput("\tError: Unexpected character in argument to -t (%s)\n", optarg);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }
            }

            if (i >= TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND)
            {
                tbDiagResponseOutput("\tError: Too many target power subbands, max is %d\n",
                                     TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND);
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
        }
        break;

        case 'b':
        {
            char *  endptr = optarg;
            uint8_t channelStart;
            uint8_t channelEnd;
            uint8_t channelCount;
            uint8_t settingCount;

            channelStart = (uint8_t)strtol(optarg, &endptr, 10);
            VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

            channelCount = (uint8_t)strtol(endptr + 1, &endptr, 10);
            VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

            settingCount = (uint8_t)strtol(endptr + 1, &endptr, 10);
            VerifyOrExit(*endptr == '\0', error = OT_ERROR_INVALID_ARGS);

            channelEnd = channelStart + channelCount - 1;

            if ((!tbWirelessCalIsChannelValid(channelStart)) || (!tbWirelessCalIsChannelValid(channelEnd)))
            {
                tbDiagResponseOutput("\tError: Channel %u or count %u are not valid\n", channelStart, channelCount);
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            if (subbandSettingIndex >= TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND)
            {
                tbDiagResponseOutput("\tError: Too many subbandSettings arguments, max is %u\n",
                                     TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND);
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            if (subbandSetting == NULL)
            {
                subbandSetting = &parameters->mSubbandSettings[0];
            }
            else
            {
                // Move to next SubBandSetting entry.
                subbandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT(subbandSetting);

                // Make sure using this subbandSetting won't write past our stack buffer.
                if ((uint8_t *)subbandSetting >= stackBufferEnd - WIRELESS_CAL_SUBBAND_SETTING_SIZE(settingCount))
                {
                    tbDiagResponseOutput("\tError: More subbandSettings than can fit in stack buffer\n");
                    ExitNow(error = OT_ERROR_NO_BUFS);
                }
            }

            subbandSettingIndex++;
            subbandSetting->mFrequencyStart   = tbWirelessCalMapFrequency(channelStart);
            subbandSetting->mFrequencyEnd     = tbWirelessCalMapFrequency(channelEnd);
            subbandSetting->mNumPowerSettings = settingCount;

            otLogDebgPlat("subbandSetting: FreqStart=%d, FreqEnd=%d NumPowerSettings=%d",
                          subbandSetting->mFrequencyStart, subbandSetting->mFrequencyEnd,
                          subbandSetting->mNumPowerSettings);
        }
        break;

        case 's':
        {
            char * endptr = optarg;
            size_t i;

            if (subbandSetting == NULL)
            {
                tbDiagResponseOutput("\tError: You need to specify a subband setting first\n");
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            for (i = 0; i < subbandSetting->mNumPowerSettings; i++)
            {
                tbWirelessCalPowerSetting *calPowerSetting = &subbandSetting->mPowerSettings[i];
                tbPowerSetting             powerSetting;
                int16_t                    calibratedPower;

                calibratedPower = (int16_t)strtol(endptr, &endptr, 10);
                VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

                powerSetting.mRadioPower = (int8_t)strtol(endptr + 1, &endptr, 10);
                VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);

                if (!tbHalRadioIsPowerValid(powerSetting.mRadioPower))
                {
                    tbDiagResponseOutput("\tError: %d is not a valid radio power in dBm value\n",
                                         powerSetting.mRadioPower);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }

                powerSetting.mFemMode = (uint8_t)strtol(endptr + 1, &endptr, 10);
                VerifyOrExit(*endptr == ',', error = OT_ERROR_INVALID_ARGS);
                if (!tbHalFemIsModeValid(powerSetting.mFemMode))
                {
                    tbDiagResponseOutput("\tError: %d is not a valid fem mode\n", powerSetting.mFemMode);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }

                powerSetting.mFemPower = (uint8_t)strtol(endptr + 1, &endptr, 10);
                if (!tbHalFemIsPowerValid(powerSetting.mFemPower))
                {
                    tbDiagResponseOutput("\tError: %d is not a valid fem power\n", powerSetting.mFemPower);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }

                calPowerSetting->mCalibratedPower = calibratedPower;
                error = tbPowerSettingsEncode(&powerSetting, &calPowerSetting->mEncodedPower);
                VerifyOrExit(error == OT_ERROR_NONE, OT_NOOP);

                otLogDebgPlat(" %d: powerSettings: EncodedPower=%d, RadioPower=%d, FemMode=%d, FemPower=%d", i,
                              calPowerSetting->mEncodedPower, powerSetting.mRadioPower, powerSetting.mFemMode,
                              powerSetting.mFemPower);

                if (*endptr == '/')
                {
                    // More entries coming.
                    endptr++;
                }
                else if (*endptr == '\0')
                {
                    break;
                }
                else
                {
                    tbDiagResponseOutput("Unexp char in arg to -s (%s)\n", optarg);
                    tbDiagResponseOutput("\tError: invalid powerSettings argument %s\n", optarg);
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                    break;
                }
            }

            VerifyOrExit(wirelessCalIsSubbandSettingValid(subbandSetting), error = OT_ERROR_INVALID_ARGS);
        }
        break;

        default:
            otLogDebgPlat("aArgsLength=%d optind=%d, optionIndex=%d", aArgsLength, optind, optionIndex);
            tbDiagResponseOutput("\tError: Unrecognized character code 0x%x\n", ch);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
            break;
        }
    }

    if ((subbandSetting == NULL) || (subbandSetting->mNumPowerSettings == 0))
    {
        tbDiagResponseOutput("\tError: Must have defined at least one subbandSetting\n");
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // If we've been given less than the maximum supported number of subbandSettings, zero
    // out one so the parsing code can find a null terminator instead of reading garbage.
    if (subbandSettingIndex < TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND)
    {
        // Move to next SubBandSetting entry.
        subbandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT(subbandSetting);

        if ((uint8_t *)subbandSetting >= stackBufferEnd - WIRELESS_CAL_SUBBAND_SETTING_SIZE(0))
        {
            tbDiagResponseOutput("\tError: More subbandSettings than can fit in stack buffer\n");
            ExitNow(error = OT_ERROR_NO_BUFS);
        }

        subbandSetting->mFrequencyStart   = 0;
        subbandSetting->mFrequencyEnd     = 0;
        subbandSetting->mNumPowerSettings = 0;
    }

    // Move to the end of the subband settings.
    subbandSetting = TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT(subbandSetting);

    // Write wireless calibration table to sysenv.
    error = tbWirelessCalSave(&calibration, (uint8_t *)subbandSetting - (uint8_t *)parameters);
    if (error != OT_ERROR_NONE)
    {
        tbDiagResponseOutput("\tError: Save calibration table to sysenv failed\n");
        ExitNow();
    }

    // Write power settings table to sysenv.
    if ((error = tbPowerSettingsSave()) != OT_ERROR_NONE)
    {
        tbDiagResponseOutput("\tError: Save power setting table to sysenv failed\n");
    }

exit:
    optind = 0;

    return error;
}

static otError processWirelessCalSet(uint8_t aArgsLength, char *aArgs[])
{
    otError                         error;
    const tbWirelessCalCalibration *calibration = wirelessCalGetCalibration();

    VerifyOrExit(calibration != NULL, error = OT_ERROR_NOT_FOUND);
    error = wirelessCalSetCalibration(calibration, aArgsLength, aArgs);

exit:
    return error;
}

static otError processWirelessCalApply(uint8_t aArgsLength, char *aArgs[])
{
    otError  error       = OT_ERROR_NONE;
    int      optionIndex = 0;
    int      ch;
    uint16_t regulatoryCode;
    uint8_t  channel;
    uint32_t frequency;
    int16_t  targetPower;
    int16_t  calibratedPower;
    int      powerAdjustment     = 0;
    bool     isRegulatoryCodeSet = false;
    bool     isChannelSet        = false;

    const struct option options[] = {
        {(char *)"r", 1, NULL, 'r'}, // Regulatory domain code.
        {(char *)"c", 1, NULL, 'c'}, // Channel.
        {(char *)"a", 1, NULL, 'a'}, // Adjustment power.
        {0, 0, 0, 0},
    };

    optind = 0;

    while ((ch = getopt_long(aArgsLength, aArgs, "r:c:a:", options, &optionIndex)) != -1)
    {
        switch (ch)
        {
        case 'r':
            regulatoryCode      = tbRegulatoryDomainEncode(optarg);
            isRegulatoryCodeSet = true;
            break;

        case 'c':
            channel      = atoi(optarg);
            isChannelSet = true;
            break;

        case 'a':
            powerAdjustment = atoi(optarg);
            break;

        default:
            tbDiagResponseOutput("\tError: Unrecognized character code 0x%x\n", ch);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
            break;
        }
    }

    if ((!isRegulatoryCodeSet) || (!isChannelSet))
    {
        tbDiagResponseOutput("\tError: Missing either regulatory code or channel\n");
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    if (!tbWirelessCalIsChannelValid(channel))
    {
        tbDiagResponseOutput("\tError: Channel %u is not a valid channel\n", channel);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    frequency = tbWirelessCalMapFrequency(channel);

    // Lookup the default target power for the given regulatory code and frequency.
    error = tbWirelessCalLookupTargetPower(regulatoryCode, frequency, &targetPower);
    if (error != OT_ERROR_NONE)
    {
        tbDiagResponseOutput("\tError: Wirelesscal lookup failed for regulatoryCode=0x%04x, Channel=%u\n",
                             regulatoryCode, channel);

        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    // Adjust the target power.
    targetPower += powerAdjustment;

    // Lookup the calibrated power for the given target power.
    error = tbWirelessCalLookupPowerSetting(frequency, targetPower, &calibratedPower, NULL);
    if (error != OT_ERROR_NONE)
    {
        tbDiagResponseOutput("\tError: Wirelesscal lookup failed for channel(%u) adjPower(%d)\n", channel,
                             powerAdjustment);
        ExitNow(error = OT_ERROR_INVALID_ARGS);
    }

    tbHalRadioSetRegulatoryTargetPower(regulatoryCode, calibratedPower);

    tbDiagResponseOutput("\tApplied adjusted Target Power(%d) on channel(%u)\n", targetPower, channel);
    tbDiagResponseOutput(
        "\tPlease make sure to run openthread <diag channel (%u)> cmd to set channel before transmitting\n", channel);

exit:
    optind = 0;
    return error;
}

otError tbDiagProcessWirelessCal(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aArgsLength > 0, OT_NOOP);

    if (strcmp("set", aArgs[0]) == 0)
    {
        error = processWirelessCalSet(aArgsLength, aArgs);
    }
    else if (strcmp("get", aArgs[0]) == 0)
    {
        error = processWirelessCalGet(aArgsLength, aArgs);
    }
    else if (strcmp("apply", aArgs[0]) == 0)
    {
        error = processWirelessCalApply(aArgsLength, aArgs);
    }
    else
    {
        otLogDebgPlat("%s: Wrong wirelesscal opcode: %s", __func__, aArgs[0]);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE && TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
