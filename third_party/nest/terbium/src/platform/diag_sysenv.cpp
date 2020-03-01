/*
 *  Copyright (c) 2020 Google LLC.
 *  All rights reserved.
 *
 *  This document is the property of Google LLC, Inc. It is
 *  considered proprietary and confidential statsrmation.
 *
 *  This document may not be reproduced or transmitted in any form,
 *  in whole or in part, without the express written permission of
 *  Google LLC.
 *
 */

/*
 * @file
 *   This file implements Sysenv diag commands.
 *
 */

#include <openthread-core-config.h>
#include <terbium-config.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <openthread/platform/diag.h>
#include <openthread/platform/toolchain.h>

#include "platform/diag.h"
#include "platform/diag_sysenv.h"
#include "platform/sysenv.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE

#define DIAG_LINE_LENGTH 80
#define DIAG_OUTPUT_MAX_HEX_STRING_LENGTH 20
#define DIAG_TEMP_BUFFER_SIZE 250

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static int Hex2Bin(const char *aHex, uint8_t *aBin, uint16_t aBinLength, bool aAllowTruncate)
{
    size_t      hexLength = strlen(aHex);
    const char *hexEnd    = aHex + hexLength;
    uint8_t *   cur       = aBin;
    uint8_t     numChars  = hexLength & 1;
    uint8_t     byte      = 0;
    int         len       = 0;
    int         rval;

    if (!aAllowTruncate)
    {
        VerifyOrExit(((hexLength + 1) / 2 <= aBinLength), rval = -1);
    }

    while (aHex < hexEnd)
    {
        if ('A' <= *aHex && *aHex <= 'F')
        {
            byte |= 10 + (*aHex - 'A');
        }
        else if ('a' <= *aHex && *aHex <= 'f')
        {
            byte |= 10 + (*aHex - 'a');
        }
        else if ('0' <= *aHex && *aHex <= '9')
        {
            byte |= *aHex - '0';
        }
        else
        {
            ExitNow(rval = -1);
        }

        aHex++;
        numChars++;

        if (numChars >= 2)
        {
            numChars = 0;
            *cur++   = byte;
            byte     = 0;
            len++;

            if (len == aBinLength)
            {
                ExitNow(rval = len);
            }
        }
        else
        {
            byte <<= 4;
        }
    }

    rval = len;

exit:
    return rval;
}

static otError setDataEnv(const char *aKey, const char *aValue)
{
    otError  error;
    uint32_t len;
    uint8_t  bin[DIAG_TEMP_BUFFER_SIZE];

    VerifyOrExit((len = Hex2Bin(aValue, bin, sizeof(bin), false)) > 0, error = OT_ERROR_FAILED);
    error = tbSysenvSet(aKey, bin, len);

exit:
    return error;
}

static otError setIntegerEnv(const char *aKey, const char *aValue)
{
    otError  error;
    uint32_t value;
    char *   endptr;

    value = static_cast<uint32_t>(strtol(aValue, &endptr, 0));
    VerifyOrExit(*endptr == '\0', error = OT_ERROR_PARSE);

    error = tbSysenvSet(aKey, &value, sizeof(value));

exit:
    return error;
}
#endif // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE

static inline bool isFormatValid(char aFormat)
{
    return ((aFormat == 'x') || (aFormat == 'd') || (aFormat == 'o')) ? true : false;
}

static inline bool isFormatLengthValid(char aFormatLength)
{
    return ((aFormatLength == '1') || (aFormatLength == '2') || (aFormatLength == '4')) ? true : false;
}

static void printData(char *aValue, uint16_t aLength, char aFormat, uint8_t aFormatLength)
{
    char lineFormat[10];
    int  i          = 0;
    int  lineLength = DIAG_LINE_LENGTH;

    snprintf(lineFormat, sizeof(lineFormat), " %%0%d%c", aFormatLength * 2, aFormat);

    while (i < aLength)
    {
        uint32_t val = 0;

        if (lineLength >= DIAG_LINE_LENGTH)
        {
            lineLength = tbDiagResponseOutput("\r\n%04x:", i);
        }

        switch (aFormatLength)
        {
        case 1:
            val = aValue[i] & 0xff;
            break;
        case 2:
            val = *(reinterpret_cast<uint16_t *>(&aValue[i])) & 0xffff;
            break;
        case 4:
            val = *(reinterpret_cast<uint32_t *>(&aValue[i]));
            break;
        default:
            val = 0;
        }

        i += aFormatLength;

        lineLength += tbDiagResponseOutput(lineFormat, val);
    }

    tbDiagResponseOutput("\r\n");
}

static void printString(const char *aString, uint16_t aLength)
{
    bool hexFormat = false;
    bool outputAll = true;

    for (size_t i = 0; i < aLength; i++)
    {
        if ((aString[i] < ' ') || (aString[i] > '~'))
        {
            hexFormat = true;
            break;
        }
    }

    if (hexFormat && (aLength > DIAG_OUTPUT_MAX_HEX_STRING_LENGTH))
    {
        aLength   = DIAG_OUTPUT_MAX_HEX_STRING_LENGTH;
        outputAll = false;
    }

    for (size_t i = 0; i < aLength; i++)
    {
        if (hexFormat)
        {
            tbDiagResponseOutput("0x%02X ", aString[i]);
        }
        else
        {
            tbDiagResponseOutput("%c", aString[i]);
        }
    }

    if (!outputAll)
    {
        tbDiagResponseOutput(" ...");
    }
}

otError tbDiagProcessSysenv(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);

    if (aArgsLength == 0)
    {
        const char *key;
        const void *value;
        uint32_t    keyLen;
        uint32_t    valueLen;
        uint32_t    iterator = TB_SYSENV_ITERATOR_INIT;

        tbDiagResponseOutput("begin sysenv dump\r\n");

        while (tbSysenvGetNext(&iterator, &key, &keyLen, &value, &valueLen) == OT_ERROR_NONE)
        {
            if (key != NULL)
            {
                printString(key, keyLen);
                tbDiagResponseOutput("=");
            }

            if (value != NULL)
            {
                printString(reinterpret_cast<const char *>(value), valueLen);
            }

            tbDiagResponseOutput("\r\n");
        }

        tbDiagResponseOutput("end sysenv dump\r\n");
    }
    else if (strcmp(aArgs[0], "check") == 0)
    {
        tbSysenvStats stats;

        if ((error = tbSysenvGetStats(&stats)) == OT_ERROR_NONE)
        {
            tbDiagResponseOutput("\tbytes: %u\r\n", stats.mTotalBytes);
            tbDiagResponseOutput("\tmax: %u\r\n", stats.mMaxBytes);
            tbDiagResponseOutput("\theader: %u\r\n", stats.mHeaderBytes);
            tbDiagResponseOutput("\tentries: %u\r\n", stats.mNumEntries);
            tbDiagResponseOutput("\tkey: %u\r\n", stats.mKeyBytes);
            tbDiagResponseOutput("\tvalue: %u\r\n", stats.mValueBytes);
            tbDiagResponseOutput("\tmeta: %u\r\n", stats.mMetadataBytes);
        }
        else
        {
            tbDiagResponseOutput("sysenv check returned error %d\r\n", error);
        }
    }
    else if (strcmp(aArgs[0], "get") == 0)
    {
        if (aArgsLength > 1 && aArgsLength < 4)
        {
            char *   key;
            char     value[DIAG_TEMP_BUFFER_SIZE] = {0};
            uint32_t len                          = DIAG_TEMP_BUFFER_SIZE;

            key = (aArgsLength == 3) ? aArgs[2] : aArgs[1];

            error = tbSysenvGet(key, value, &len);

            if (error == OT_ERROR_NONE)
            {
                if (aArgsLength == 3)
                {
                    if (strcmp(aArgs[1], "--len") == 0)
                    {
                        printString(value, len);
                        tbDiagResponseOutput("\r\n[length:%d]\r\n", strlen(value));
                    }
                    else if (strcmp(aArgs[1], "--int") == 0)
                    {
                        tbDiagResponseOutput("%d\r\n", *(int *)value);
                    }
                    else if (strcmp(aArgs[1], "--data") == 0)
                    {
                        printData(value, len, 'x', 1);
                    }
                    else if (strncmp(aArgs[1], "--data=", 7) == 0)
                    {
                        char    fmt    = aArgs[1][7];
                        uint8_t fmtLen = aArgs[1][8];

                        if (!isFormatValid(fmt))
                        {
                            tbDiagResponseOutput("Unknown format: %c\r\n", fmt);
                            ExitNow();
                        }

                        if (!isFormatLengthValid(fmtLen))
                        {
                            tbDiagResponseOutput("Invalid length: %c\r\n", fmtLen);
                            ExitNow();
                        }

                        printData(value, len, fmt, fmtLen - '0');
                    }
                }
                else
                {
                    printString(value, len);
                    tbDiagResponseOutput("\r\n");
                }
            }
            else
            {
                tbDiagResponseOutput("failed to get value for key %s (%d)\r\n", key, error);
            }
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
            tbDiagResponseOutput("incorrect number of arguments (%d) for get command\r\n", aArgsLength);
        }
    }
#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
    else if (strcmp(aArgs[0], "set") == 0)
    {
        if (aArgsLength > 1 && aArgsLength < 5)
        {
            const char *key;
            const char *value;

            if (aArgsLength > 3)
            {
                key   = aArgs[2];
                value = aArgs[3];

                if (strcmp(aArgs[1], "--data") == 0)
                {
                    error = setDataEnv(key, value);
                }
                else if (strcmp(aArgs[1], "--int") == 0)
                {
                    error = setIntegerEnv(key, value);
                }
                else
                {
                    tbDiagResponseOutput("Unknown data format, valid ones include (--data |--int)\r\n");
                    error = OT_ERROR_INVALID_ARGS;
                }

                if (error == OT_ERROR_NONE)
                {
                    tbDiagResponseOutput("successfully set key %s: %s\r\n", key, value);
                }
                else
                {
                    tbDiagResponseOutput("failed to set key %s: %d\r\n", key, error);
                }
            }
            else
            {
                key = aArgs[1];

                if (aArgsLength == 3)
                {
                    value = aArgs[2];
                }
                else
                {
                    value = NULL;
                }

                error = tbSysenvSet(key, value, value == NULL ? 0 : strlen(value));

                if (error != OT_ERROR_NONE)
                {
                    tbDiagResponseOutput("failed to set key %s\r\n", key);
                }
                else
                {
                    if (value == NULL)
                    {
                        tbDiagResponseOutput("successfully deleted key %s\r\n", key);
                    }
                    else
                    {
                        tbDiagResponseOutput("successfully set key %s: %s\r\n", key, value);
                    }
                }
            }
        }
        else
        {
            error = OT_ERROR_INVALID_ARGS;
            tbDiagResponseOutput("incorrect number of arguments (%d) for set command\r\n", aArgsLength);
        }
    }
#endif // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
    else
    {
        error = OT_ERROR_INVALID_COMMAND;
        tbDiagResponseOutput("Unknown command '%s'\r\n", aArgs[0]);
    }

exit:
    return error;
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
