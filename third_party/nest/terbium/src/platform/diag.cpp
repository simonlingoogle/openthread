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
 *   This file implements the OpenThread platform abstraction for the diag.
 *
 */

#include <openthread-core-config.h>
#include <terbium-config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <openthread/platform/diag.h>

#include "platform/diag.h"
#include "platform/diag_peripheral.h"
#include "platform/diag_sysenv.h"
#include "platform/diag_wireless_cal.h"
#include "platform/wireless_cal.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE
static bool   sDiagMode;
static char * sOutput;
static size_t sOutputMaxLen;

int tbDiagResponseOutput(const char *aFormat, ...)
{
    va_list args;
    int     len = 0;

    VerifyOrExit(sOutputMaxLen > 0, OT_NOOP);

    va_start(args, aFormat);

    if ((len = vsnprintf(sOutput, sOutputMaxLen, aFormat, args)) > 0)
    {
        sOutputMaxLen -= len;
        sOutput += len;
    }

    va_end(args);

exit:
    return len;
}

otError otPlatDiagProcess(otInstance *aInstance,
                          uint8_t     aArgsLength,
                          char *      aArgs[],
                          char *      aOutput,
                          size_t      aOutputMaxLen)
{
    otError error = OT_ERROR_INVALID_COMMAND;

    aOutput[0]    = '\0'; // In case the previous data is not erased and diag command outputs nothing.
    sOutput       = aOutput;
    sOutputMaxLen = aOutputMaxLen;

    if (sDiagMode)
    {
        if ((aArgsLength > 1) && strcmp(aArgs[0], "factory") == 0)
        {
            if (strcmp(aArgs[1], "sysenv") == 0)
            {
                error = tbDiagProcessSysenv(aInstance, aArgsLength - 2, &aArgs[2]);
            }
#if TERBIUM_CONFIG_WIRELESS_CALIBRATION_ENABLE
            else if (strcmp(aArgs[1], "wirelesscal") == 0)
            {
                error = tbDiagProcessWirelessCal(aInstance, aArgsLength - 2, &aArgs[2]);
            }
#endif
        }
        else
        {
            error = tbDiagProcessPeripheral(aInstance, aArgsLength, aArgs);
        }
    }
    else
    {
        tbDiagResponseOutput("failed\r\ndiagnostics mode is disabled\r\n");
    }

    return error;
}

void otPlatDiagModeSet(bool aMode)
{
    sDiagMode = aMode;
}

bool otPlatDiagModeGet(void)
{
    return sDiagMode;
}

void otPlatDiagChannelSet(uint8_t aChannel)
{
    OT_UNUSED_VARIABLE(aChannel);
}

void otPlatDiagTxPowerSet(int8_t aTxPower)
{
    otPlatRadioSetTransmitPower(NULL, aTxPower);
}

void otPlatDiagRadioReceived(otInstance *aInstance, otRadioFrame *aFrame, otError aError)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);
    OT_UNUSED_VARIABLE(aError);
}

void otPlatDiagAlarmCallback(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    if (sDiagMode)
    {
        tbDiagAlarmCallback(aInstance);
    }
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
