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
 *   This file implements peripheral diag commands.
 *
 */

#include <openthread-core-config.h>
#include <terbium-config.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <radio/radio.hpp>
#include <openthread/error.h>
#include <openthread/instance.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/toolchain.h>

#include "chips/include/delay.h"
#include "chips/include/fem.h"
#include "chips/include/gpio.h"
#include "chips/include/radio.h"
#include "chips/include/watchdog.h"
#include "platform/backtrace.h"
#include "platform/diag.h"
#include "platform/diag_peripheral.h"
#include "platform/wireless_cal.h"

#if OPENTHREAD_CONFIG_DIAG_ENABLE
static otError processGpio(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processFemPower(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processTxPower(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processCw(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processTransmit(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
static otError processWatchdog(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
#if TERBIUM_CONFIG_BACKTRACE_ENABLE
static otError processBacktrace(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
#endif

struct DiagCommand
{
    const char *mName;
    otError (*mCommand)(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);
};

static const struct DiagCommand sCommands[] = {
    {"gpio", &processGpio},          {"vcc2", &processFemPower}, {"pwrIndex", &processFemPower},
    {"txPower", &processTxPower},    {"cw", &processCw},         {"transmit", &processTransmit},
    {"watchdog", &processWatchdog},
#if TERBIUM_CONFIG_BACKTRACE_ENABLE
    {"backtrace", &processBacktrace}
#endif
};

static volatile uint32_t sIntCounter;
static int32_t           sTxNumPackets;
static uint32_t          sTxPeriodMs;
static bool              sTxUserPayload;
static otRadioFrame *    sTxPacket;

static void gpioIntCallback(const tbHalGpioPin aPin)
{
    OT_UNUSED_VARIABLE(aPin);
    sIntCounter++;
}

static otError processGpio(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError     error = OT_ERROR_NONE;
    static bool intInitialized;

    OT_UNUSED_VARIABLE(aInstance);

    if ((aArgsLength == 2) && (strcmp(aArgs[0], "get") == 0))
    {
        tbHalGpioPin pin = static_cast<tbHalGpioPin>(atoi(aArgs[1]));

        VerifyOrExit(tbHalGpioIsValid(pin), error = OT_ERROR_INVALID_ARGS);

        tbHalGpioInputRequest(pin, TB_HAL_GPIO_PULL_NONE);
        tbDiagResponseOutput("%d\r\n", tbHalGpioGetValue(pin));
        tbHalGpioRelease(pin);
    }
    else if ((aArgsLength == 3) && (strcmp(aArgs[0], "set") == 0))
    {
        tbHalGpioPin   pin   = static_cast<tbHalGpioPin>(atoi(aArgs[1]));
        tbHalGpioValue value = (atoi(aArgs[2]) == 0) ? TB_HAL_GPIO_VALUE_LOW : TB_HAL_GPIO_VALUE_HIGH;

        VerifyOrExit(tbHalGpioIsValid(pin), error = OT_ERROR_INVALID_ARGS);
        tbHalGpioOutputRequest(pin, value);
    }
    else if (((aArgsLength == 2) || (aArgsLength == 3)) && (strcmp(aArgs[0], "int") == 0))
    {
        tbHalGpioPin pin = static_cast<tbHalGpioPin>(atoi(aArgs[1]));

        VerifyOrExit(tbHalGpioIsValid(pin), error = OT_ERROR_INVALID_ARGS);

        if ((aArgsLength == 3) && (strcmp(aArgs[2], "release") == 0))
        {
            tbHalGpioIntRelease(pin);

            intInitialized = false;
            sIntCounter    = 0;
        }
        else if (aArgsLength == 3)
        {
            if (!intInitialized)
            {
                tbHalGpioIntMode mode = TB_HAL_GPIO_INT_MODE_BOTH_EDGES;

                if (strcmp(aArgs[2], "rising") == 0)
                {
                    mode = TB_HAL_GPIO_INT_MODE_RISING;
                }
                else if (strcmp(aArgs[2], "falling") == 0)
                {
                    mode = TB_HAL_GPIO_INT_MODE_FALLING;
                }
                else if (strcmp(aArgs[2], "both") == 0)
                {
                    mode = TB_HAL_GPIO_INT_MODE_BOTH_EDGES;
                }
                else
                {
                    ExitNow(error = OT_ERROR_INVALID_ARGS);
                }

                tbHalGpioIntRequest(pin, TB_HAL_GPIO_PULL_PULLUP, mode, gpioIntCallback);

                intInitialized = true;
            }
            else
            {
                tbDiagResponseOutput("Busy\r\n");
                error = OT_ERROR_INVALID_STATE;
            }
        }
        else
        {
            tbDiagResponseOutput("IntCounter: %ld\r\n", sIntCounter);
        }
    }
    else if ((aArgsLength == 4) && (strcmp(aArgs[0], "loopback") == 0))
    {
        // Note:  User should connect the ouput pin and interrupt pin together
        //        before running the command "loopback".
        if (!intInitialized)
        {
            tbHalGpioIntMode mode      = TB_HAL_GPIO_INT_MODE_BOTH_EDGES;
            tbHalGpioPin     outputPin = static_cast<tbHalGpioPin>(atoi(aArgs[1]));
            tbHalGpioPin     intPin    = static_cast<tbHalGpioPin>(atoi(aArgs[2]));

            VerifyOrExit(tbHalGpioIsValid(outputPin), error = OT_ERROR_INVALID_ARGS);
            VerifyOrExit(tbHalGpioIsValid(intPin), error = OT_ERROR_INVALID_ARGS);

            if (strcmp(aArgs[3], "rising") == 0)
            {
                mode = TB_HAL_GPIO_INT_MODE_RISING;
            }
            else if (strcmp(aArgs[3], "falling") == 0)
            {
                mode = TB_HAL_GPIO_INT_MODE_FALLING;
            }
            else if (strcmp(aArgs[3], "both") == 0)
            {
                mode = TB_HAL_GPIO_INT_MODE_BOTH_EDGES;
            }
            else
            {
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            tbHalGpioOutputRequest(outputPin, TB_HAL_GPIO_VALUE_LOW);
            tbHalGpioIntRequest(intPin, TB_HAL_GPIO_PULL_PULLUP, mode, gpioIntCallback);

            for (uint32_t i = 0; i < 1000; i++)
            {
                tbHalDelayMs(1);
                tbHalGpioSetValue(outputPin, TB_HAL_GPIO_VALUE_HIGH);
                tbHalDelayMs(1);
                tbHalGpioSetValue(outputPin, TB_HAL_GPIO_VALUE_LOW);
            }

            // Delay 1 ms to guarantee that the last falling edge can be captured.
            tbHalDelayMs(1);

            tbHalGpioIntRelease(intPin);
            tbHalGpioRelease(outputPin);

            tbDiagResponseOutput("IntCounter: %ld\r\n", sIntCounter);
            sIntCounter = 0;
        }
        else
        {
            tbDiagResponseOutput("Busy\r\n");
            error = OT_ERROR_INVALID_STATE;
        }
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

static const char *diagGetFemPowerCommand(void)
{
    enum
    {
        kNumFemPowerCommand  = 2,
        kFemPowerCommandSize = 10,
    };
    static const char femPowerCommands[kNumFemPowerCommand][kFemPowerCommandSize] = {"vcc2", "pwrIndex"};

    // Find which command is supported by the current FEM.
    uint8_t index = (tbHalFemGetPowerType() == TB_HAL_FEM_POWER_TYPE_VCC2) ? 0 : 1;

    return femPowerCommands[index];
}

static char diagGetFemPowerIdentifier(void)
{
    return (tbHalFemGetPowerType() == TB_HAL_FEM_POWER_TYPE_VCC2) ? 'v' : 'i';
}

static otError processFemPower(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError     error              = OT_ERROR_INVALID_ARGS;
    const char *femPowerCommand    = diagGetFemPowerCommand();
    char        femPowerIdentifier = diagGetFemPowerIdentifier();
    uint8_t     outFemPower;
    uint8_t     inFemPower;

    OT_UNUSED_VARIABLE(aInstance);

    // Check whether FEM supports this FEM diag command.
    VerifyOrExit(strcmp(femPowerCommand, aArgs[-1]) == 0);

    VerifyOrExit(aArgsLength == 1);

    // Argument is in format pXXX where p is 'v' or 'i' and XXX is the FEM power setting.
    VerifyOrExit(*aArgs[0] == femPowerIdentifier);

    // +1 skips the identifier 'v' or 'i'.
    outFemPower = strtol(aArgs[0] + 1, NULL, 10);

    VerifyOrExit(tbHalFemIsPowerValid(outFemPower));
    VerifyOrExit((error = tbHalFemSetPower(outFemPower)) == OT_ERROR_NONE);
    VerifyOrExit((error = tbHalFemGetPower(&inFemPower)) == OT_ERROR_NONE);

    if (tbHalFemGetPowerType() == TB_HAL_FEM_POWER_TYPE_VCC2)
    {
        tbDiagResponseOutput("Set %s voltage value succeeded: %d, %d\n", aArgs[0], outFemPower, inFemPower);
    }
    else
    {
        tbDiagResponseOutput("Set %s power index value succeeded: %d, %d\n", aArgs[0], outFemPower, inFemPower);
    }

    error = OT_ERROR_NONE;

exit:
    if (error != OT_ERROR_NONE)
    {
        uint8_t minFemPower;
        uint8_t maxFemPower;

        assert(tbHalFemGetPowerRange(&minFemPower, &maxFemPower) == OT_ERROR_NONE);

        tbDiagResponseOutput("Error: Illegal arguments\n"
                             "Usage:\n"
                             "   %s <value>\n"
                             "   <value>:%c%d ~ %c%d\n",
                             femPowerCommand, femPowerIdentifier, minFemPower, femPowerIdentifier, maxFemPower);
    }

    return error;
}

static otError processTxPower(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError  error = OT_ERROR_NONE;
    int16_t  targetPower;
    uint16_t regulatoryCode;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aArgsLength == 2, error = OT_ERROR_INVALID_ARGS);

    regulatoryCode = atoi(aArgs[0]);
    targetPower    = atoi(aArgs[1]);

    tbDiagResponseOutput("Setting PowerCal override reg_code(%u) target_power(%d)\n", regulatoryCode, targetPower);
    tbHalRadioSetRegulatoryTargetPower(regulatoryCode, targetPower);

exit:
    return error;
}

static otError processCw(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aArgsLength == 1, error = OT_ERROR_INVALID_ARGS);

    if (strcmp(aArgs[0], "stop") == 0)
    {
        uint8_t tempChannel;
        uint8_t channel = tbHalRadioGetChannel();

        // Diags CW mode happens under the covers of the radio state machine.
        // That is, when we enter CW mode the radio state machine still indicates
        // that the radio is in receive mode.  To exit CW mode we need to force
        // radio to make the appropriate calls to put the radio into receive mode.
        // The radio driver will only make the calls to put the radio into receive
        // mode if either the radio mode has changed or the channel has changed.
        // Since the radio driver believes the radio to be in receive mode when
        // we are actually in diags CW mode we need to change the channel to force
        // otPlatRadioReceive() to actually put the driver into receive mode thereby
        // stopping CW mode.
        tempChannel = (channel == ot::Radio::kChannelMax) ? ot::Radio::kChannelMin : (channel + 1);

        otPlatRadioReceive(aInstance, tempChannel);
        otPlatRadioReceive(aInstance, channel);

        tbDiagResponseOutput("Stopped transmitting CW on channel %d\n", channel);
    }
    else if (strcmp(aArgs[0], "start") == 0)
    {
        if ((error = tbHalRadioEmitContinuousCarrier()) == OT_ERROR_NONE)
        {
            uint8_t channel = tbHalRadioGetChannel();

            tbDiagResponseOutput("Started transmitting CW on channel %d\n", channel);
        }
        else
        {
            tbDiagResponseOutput("Failed to enter continuous carrier mode.  Try again.\n");
        }
    }

exit:
    return error;
}

static otError processTransmit(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    // Similar to the generic OT diags cmd "send" and "repeat" except all the
    // needed information for the transmit is given on the cmd line, including
    // vcc2 for FEM + tx_power encoded together as the power argument to
    // otPlatRadioTransmit(), channel, count, interval, payload. Count is the
    // number of packets to transmit (-1 for infinite). Interval is the time in
    // milliseconds between packets. Payload is a string of hex digits, two
    // characters per octet. The number of octets in the payload string is the
    // length of the payload. A negative payload number means send a generated
    // number of bytes as the payload instead of using the user specified payload.

    otError error = OT_ERROR_NONE;

    if ((aArgsLength == 1) && (strcmp("stop", aArgs[0]) == 0))
    {
        if (sTxNumPackets)
        {
            otPlatAlarmMilliStop(aInstance);

            sTxPeriodMs   = 0;
            sTxNumPackets = 0;

            tbDiagResponseOutput("Stopped previous transmit operation\n");
        }
        else
        {
            tbDiagResponseOutput("Error: transmit not in progress\n");
        }
    }
    else if (aArgsLength == 7)
    {
        // Command format: transmit <FEM_MODE> <FEM_POWER> <RADIO_POWER> <CHANNEL> <COUNT> <DELAY_MS> <LENGTH> <PAYLOAD>
        enum
        {
            kInfiniteNumPackets = -1,
            kRadioFrameCrcSize  = 2,
            kMaxPayloadLength   = OT_RADIO_FRAME_MAX_SIZE - kRadioFrameCrcSize,
        };
        uint8_t femMode  = strtol(aArgs[0], NULL, 10);
        uint8_t femPower = strtol(aArgs[1] + 1, NULL, 10); // Argument is in format xXXX where x is the
                                                           // parameter and XXX is the setting, +1 skips the 'v' or 'i'.
        int8_t      radioPower    = strtol(aArgs[2], NULL, 10);
        uint8_t     channel       = strtol(aArgs[3], NULL, 10);
        int         numPackets    = strtol(aArgs[4], NULL, 10);
        int         periodMs      = strtoul(aArgs[5], NULL, 10);
        const char *payloadString = aArgs[6];
        int         packetLength  = 0;
        uint8_t     tempChannel;

        if (sTxNumPackets)
        {
            tbDiagResponseOutput("Error: repeating transmit already active, stop first\n");
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if ((numPackets < 1) && (numPackets != kInfiniteNumPackets))
        {
            tbDiagResponseOutput("Error: numPackets must be 1 or more, or -1 for infinite\n");
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if (!tbHalFemIsModeValid(femMode))
        {
            uint8_t minMode;
            uint8_t maxMode;

            assert(tbHalFemGetModeRange(&minMode, &maxMode) == OT_ERROR_NONE);

            tbDiagResponseOutput("Error: Valid FEM mode(CHL) must be between %d and %d\n", minMode, maxMode);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if (!tbHalFemIsPowerValid(femPower))
        {
            uint8_t minFemPower;
            uint8_t maxFemPower;

            assert(tbHalFemGetPowerRange(&minFemPower, &maxFemPower) == OT_ERROR_NONE);

            tbDiagResponseOutput("Error: %s must be between %d and %d\n", diagGetFemPowerCommand(), minFemPower,
                                 maxFemPower);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if (!tbHalRadioIsPowerValid(radioPower))
        {
            uint8_t       numPowers;
            const int8_t *powerList = tbHalRadioGetSupportedPowerList(&numPowers);

            assert(powerList != NULL);

            tbDiagResponseOutput("Error: Radio power must be ");
            for (size_t i = 0; i < numPowers; i++)
            {
                tbDiagResponseOutput("%d ", powerList[i]);
            }

            tbDiagResponseOutput("\n");

            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        if ((channel < ot::Radio::kChannelMin) || (channel > ot::Radio::kChannelMax))
        {
            tbDiagResponseOutput("Error: Channel must be between %d and %d\n", ot::Radio::kChannelMin,
                                 ot::Radio::kChannelMax);
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        // Set factory power settings.
        tbHalRadioSetFactoryPowerSettings(femMode, femPower, radioPower);

        sTxPacket = otPlatRadioGetTransmitBuffer(aInstance);

        // Payload is expected to be an ascii hex string, or a single negative
        // integer that specifies the number of octets to send in a generated
        // payload.
        if (payloadString[0] == '-')
        {
            packetLength = strtol(&payloadString[1], NULL, 10);

            if ((packetLength < kRadioFrameCrcSize) || (packetLength > OT_RADIO_FRAME_MAX_SIZE))
            {
                tbDiagResponseOutput("Error: payload length %d is invald, it should be between %d and %d\n",
                                     packetLength, kRadioFrameCrcSize, OT_RADIO_FRAME_MAX_SIZE);
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            // Last 2 octets of packetLength is CRC added by radio driver.
            for (uint8_t i = 0; i < (packetLength - kRadioFrameCrcSize); i++)
            {
                sTxPacket->mPsdu[i] = i;
            }

            sTxUserPayload = false;
        }
        else
        {
            char   buf[3]        = {0};
            size_t payloadLength = strlen(payloadString) / 2; // Two characters converts to one hex digit.

            if (payloadLength == 0)
            {
                tbDiagResponseOutput("Error: no valid payload provided\n");
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }
            else if (payloadLength > kMaxPayloadLength)
            {
                tbDiagResponseOutput("Error: payload string %d is too long, max is %d octets\n", payloadLength,
                                     kMaxPayloadLength);
                ExitNow(error = OT_ERROR_INVALID_ARGS);
            }

            payloadLength = 0;

            while (*payloadString)
            {
                // Convert two characters at a time to hex digits for the psdu.
                buf[0] = *payloadString++;
                buf[1] = *payloadString++;

                sTxPacket->mPsdu[payloadLength++] = strtoul(buf, NULL, 16);
            }

            packetLength   = payloadLength + kRadioFrameCrcSize;
            sTxUserPayload = true;
        }

        sTxPacket->mChannel = channel;
        sTxPacket->mLength  = packetLength;

        // With each diag transmit command we could have a diff set of FEM
        // settings for the current channel which needs to be applied before
        // transmit. However, radio driver channel caching feature avoids the
        // use of these FEM settings if the incoming channel matches the cached
        // channel. So to bypass this radio driver behavior, create a temporary
        // channel change by going into adjecent channel.
        tempChannel = (channel == ot::Radio::kChannelMax) ? ot::Radio::kChannelMin : (channel + 1);
        otPlatRadioReceive(aInstance, tempChannel);

        if (periodMs == 0)
        {
            tbDiagResponseOutput("Error: repeat period must be non-zero\n");
            ExitNow(error = OT_ERROR_INVALID_ARGS);
        }

        sTxPeriodMs   = periodMs;
        sTxNumPackets = numPackets;
        otPlatAlarmMilliStartAt(aInstance, otPlatAlarmMilliGetNow(), 0);

        if (numPackets == kInfiniteNumPackets)
        {
            tbDiagResponseOutput(
                "sending infinite packets of length %u, of %s payload, channel %d, with delay of %u ms\r\n",
                sTxPacket->mLength, sTxUserPayload ? "user provided" : "auto generated", sTxPacket->mChannel,
                sTxPeriodMs);
        }
        else
        {
            tbDiagResponseOutput("sending %u packets of length %u, of %s payload, channel %d, with delay of %u ms\r\n",
                                 numPackets, sTxPacket->mLength, sTxUserPayload ? "user provided" : "auto generated",
                                 sTxPacket->mChannel, sTxPeriodMs);
        }
    }
    else
    {
        // Print help info.
        uint8_t     minFemPower;
        uint8_t     maxFemPower;
        uint8_t     avgFemPower;
        const char *powerCmd;
        char        powerIdentifier;

        assert(tbHalFemGetPowerRange(&minFemPower, &maxFemPower) == OT_ERROR_NONE);

        avgFemPower     = (uint8_t)(((uint16_t)minFemPower + (uint16_t)maxFemPower) / 2);
        powerCmd        = diagGetFemPowerCommand();
        powerIdentifier = diagGetFemPowerIdentifier();

        tbDiagResponseOutput("Usage:\ttransmit [stop | <chl> <%s> <txpwr> <chan> <#pkts> <repeat_ms> <payload>]\n"
                             "\te.g. transmit 0 %c%u 0 11 -1 16 -64\n"
                             "\te.g. transmit 0 %c%u 0 11 100 100 000c000000560a00010030b418\n",
                             powerCmd, powerIdentifier, avgFemPower, powerIdentifier, avgFemPower);
    }

exit:
    return error;
}

static otError processWatchdog(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(aArgsLength <= 1, error = OT_ERROR_INVALID_ARGS);

    if (aArgsLength == 0)
    {
        tbDiagResponseOutput("%s\r\n", tbHalWatchdogIsEnabled() ? "enabled" : "disabled");
    }
    else if (strcmp("enable", aArgs[0]) == 0)
    {
        tbHalWatchdogSetEnabled(true);
    }
    else if (strcmp("disable", aArgs[0]) == 0)
    {
        tbHalWatchdogSetEnabled(false);
    }
    else if (strcmp("refresh", aArgs[0]) == 0)
    {
        tbHalWatchdogRefresh();
    }
    else if (strcmp("stop", aArgs[0]) == 0)
    {
        // Wait for watchdog to reset device.
        for (;;)
            ;
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

exit:
    return error;
}

#if TERBIUM_CONFIG_BACKTRACE_ENABLE
static otError processBacktrace(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;

    OT_UNUSED_VARIABLE(aInstance);

    if (aArgsLength == 0)
    {
        uint16_t levels;
        uint16_t i;
        uint32_t buffer[20];
        char     string[200];
        char *   start = string;
        char *   end   = string + sizeof(string);

        levels = tbBacktrace(buffer, sizeof(buffer));

        for (i = 0; i < levels; i++)
        {
            start += snprintf(start, end - start, "0x%lx ", buffer[i]);
        }

        tbDiagResponseOutput("%s\r\n", string);
    }
    else if ((aArgsLength == 1) && (strcmp("memfault", aArgs[0]) == 0))
    {
        // Create an invalid function pointer and call it. This will trigger a Hard Fault.
        void (*fp)(void) = (void (*)(void))(0xffffffff);
        fp();
    }
    else if ((aArgsLength >= 1) && (strcmp("usagefault", aArgs[0]) == 0))
    {
        // Add "volatile" to prevent the compiler from optimizing the code.
        volatile uint32_t a = 10;
        volatile uint32_t b = 0;
        volatile uint32_t c;

        // This will trigger a Divide By Zero Usage Fault.
        c = a / b;

        OT_UNUSED_VARIABLE(c);
    }
    else
    {
        error = OT_ERROR_INVALID_ARGS;
    }

    return error;
}
#endif // TERBIUM_CONFIG_BACKTRACE_ENABLE

void tbDiagAlarmCallback(otInstance *aInstance)
{
    // Check if we were cancelled.
    VerifyOrExit(sTxNumPackets);

    if (sTxUserPayload)
    {
        enum
        {
            kIeee802154DsnOffset = 2,
        };
        uint8_t *seqNum = sTxPacket->mPsdu + kIeee802154DsnOffset;

        // Increment the seq_num in the user payload to help distinguish them
        // for packet error rate testing. Don't increment the seqNum for
        // generated packets that RF team typically uses for factory calibration.
        *seqNum = *seqNum + 1;
    }

    // Instruct radio driver to skip power calibration table lookup and use
    // the hardcoded factory power settings to send packets.
    tbHalRadioUseFactoryPowerSettings();

    // Start transmit packet.
    otPlatRadioTransmit(aInstance, sTxPacket);

    // Reset the use of hardcoded factory power settings to restore the power calibration table lookup.
    tbHalRadioResetFactoryPowerSettings();

    if (sTxNumPackets > 0)
    {
        sTxNumPackets--;
    }

    if (sTxNumPackets)
    {
        // Schedule for next packet.
        otPlatAlarmMilliStartAt(aInstance, otPlatAlarmMilliGetNow(), sTxPeriodMs);
    }

exit:
    return;
}

otError tbDiagProcessPeripheral(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_INVALID_COMMAND;
    uint8_t i;

    for (i = 0; i < OT_ARRAY_LENGTH(sCommands); i++)
    {
        if (strcmp(aArgs[0], sCommands[i].mName) == 0)
        {
            error = sCommands[i].mCommand(aInstance, aArgsLength - 1, aArgsLength > 1 ? &aArgs[1] : NULL);
            break;
        }
    }

    return error;
}
#endif // OPENTHREAD_CONFIG_DIAG_ENABLE
