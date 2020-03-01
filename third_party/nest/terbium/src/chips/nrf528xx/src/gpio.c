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
 *   This file includes the hardware abstraction layer for GPIO.
 *
 */

#include <board-config.h>

#include <assert.h>
#include <stddef.h>
#include <utils/code_utils.h>
#include <openthread/platform/toolchain.h>

#include <nrfx/mdk/nrf.h>

#include "chips/include/cmsis.h"
#include "chips/include/delay.h"
#include "chips/include/gpio.h"

/*
 * This file is a driver for GPIO management on Nordic NRF52x.
 * GPIOs can be controlled via two different register controls.
 * One, called GPIO, can be used to configure direction, pull,
 * set value, get value, etc. It also has SENSE configuration
 * and a LATCH register which can be used to detect changes in
 * a GPIO pin. However, the GPIO controller has no interrupt
 * generation ability.
 * The other controller, called GPIOTE, can be used to also
 * configure GPIO pins and also generate interrupts on pin
 * changes. It is limited to 8 channels though, and when a
 * channel is configured to detect input changes on a particular
 * pin, the GPIOTE consumes extra power.
 * The GPIOTE also has a PORT event/interrupt, which is tied
 * to the DETECT signal from the GPIO controller. Using the
 * PORT event/interrupt and LATCH registers, we can implement
 * interrupt per pin without the GPIOTE channels and save some power.
 * It's a bit more complicated to do edge interrupts than level
 * interrupts because the HW doesn't do this for us, but I think it's
 * possible to do edge detection in SW with a few more interrupts than
 * if the HW could do it.
 *
 * There's mention in the documentation that using the channel events
 * requires a high frequency clock, wherease the PORT detection mechanism
 * uses only a low frequency clock.  The datasheet doesn't mention any of
 * this however, so it's something we might want to bring up.
 */

#define GPIO_MAX_NUM_INTERRUPTS 4                              ///< Maximum number of interrupt pins.
#define GPIO_GET_PORT(x) (((x) >> 5) & 0x07)                   ///< Get the port number.
#define GPIO_GET_PIN(x) ((x)&0x1f)                             ///< Get the pin number.
#define GPIO_PIN(aPort, aPin) (((aPort) << 5) | ((aPin)&0x1f)) ///< Genarate a GPIO pin.

#if defined(NRF52840_XXAA)
#include <nrfx/mdk/nrf52840_peripherals.h>
static NRF_GPIO_Type *sGpios[GPIO_COUNT]   = {NRF_P0, NRF_P1};
static const uint8_t  sNumPins[GPIO_COUNT] = {P0_PIN_NUM, P1_PIN_NUM};
#elif defined(NRF52811_XXAA)
#include <nrfx/mdk/nrf52811_peripherals.h>
static NRF_GPIO_Type *sGpios[GPIO_COUNT]   = {NRF_P0};
static const uint8_t  sNumPins[GPIO_COUNT] = {P0_PIN_NUM};
#endif

/**
 * This enumeration defines the GPIO directions.
 */
typedef enum
{
    kGpioDirectionInput  = 0, ///< Input floating mode.
    kGpioDirectionOutput = 1, ///< Output push pull mode.
} GpioDirection;

/**
 * This structure represents interrupt pin related parameters.
 */
typedef struct IntEntry
{
    tbHalGpioIntCallback mCallback; ///< Callback function.
    tbHalGpioPin         mIntPin;   ///< Interrupt pin.
    tbHalGpioIntMode     mIntMode;  ///< Interrupt mode.
} IntEntry;

// Interrupt pin table.
static IntEntry sIntEntries[GPIO_MAX_NUM_INTERRUPTS];

// Terbium GPIO pull mode to nrf528xx pull mode mapping table.
static const uint8_t sPullTable[] = {GPIO_PIN_CNF_PULL_Disabled, GPIO_PIN_CNF_PULL_Pulldown, GPIO_PIN_CNF_PULL_Pullup};

static IntEntry *gpioFindEmptyIntEntry(void)
{
    IntEntry *entry = NULL;

    for (uint8_t i = 0; i < GPIO_MAX_NUM_INTERRUPTS; i++)
    {
        if (sIntEntries[i].mCallback == NULL)
        {
            entry = &sIntEntries[i];
            break;
        }
    }

    return entry;
}

static IntEntry *gpioFindIntEntry(tbHalGpioPin aPin)
{
    IntEntry *entry = NULL;

    for (uint8_t i = 0; i < GPIO_MAX_NUM_INTERRUPTS; i++)
    {
        if (sIntEntries[i].mIntPin == aPin)
        {
            entry = &sIntEntries[i];
            break;
        }
    }

    return entry;
}

static void gpioFreeIntEntry(tbHalGpioPin aPin)
{
    IntEntry *entry = gpioFindIntEntry(aPin);

    if (entry != NULL)
    {
        entry->mCallback = NULL;
    }
}

static inline uint32_t gpioGetPullFlag(tbHalGpioPull aPull)
{
    assert(aPull < sizeof(sPullTable));
    return sPullTable[aPull];
}

static inline bool gpioIsPinValid(tbHalGpioPin aPin)
{
    uint8_t port = GPIO_GET_PORT(aPin);
    return ((port < GPIO_COUNT) && (GPIO_GET_PIN(aPin) < sNumPins[port]));
}

void tbHalGpioInit(void)
{
    uint8_t i;

    // Using LDETECT mode, a PORT event is generated on a rising edge of LDETECT
    // which can be generated under the following conditions:
    // 1) When the LATCH register has an initial value of 0, and then any LATCH
    //    bit is set.
    // 2) When the LATCH register has a non-zero value after attempting to clear
    //    any of its bits.
    for (i = 0; i < GPIO_COUNT; i++)
    {
        sGpios[i]->DETECTMODE = GPIO_DETECTMODE_DETECTMODE_LDETECT;
    }

    // Enable NVIC interrupt for GPIOTE.  Nothing should happen until PORT
    // interrupt is triggered by SENSE being configured.
    NVIC_SetPriority(GPIOTE_IRQn, GPIO_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);
    NVIC_EnableIRQ(GPIOTE_IRQn);

    // Enable PORT interrupt in GPIOTE.
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Set << GPIOTE_INTENSET_PORT_Pos;
}

static inline NRF_GPIO_Type *gpioGet(tbHalGpioPin aPin)
{
    assert(gpioIsPinValid(aPin));
    return sGpios[GPIO_GET_PORT(aPin)];
}

static inline void gpioSetOutput(NRF_GPIO_Type *aGpio, uint8_t aPin, tbHalGpioValue aValue)
{
    if (aValue == TB_HAL_GPIO_VALUE_LOW)
    {
        aGpio->OUTCLR = (1UL << aPin);
    }
    else
    {
        aGpio->OUTSET = (1UL << aPin);
    }
}

static void gpioRequest(tbHalGpioPin aPin, GpioDirection aDirection, tbHalGpioPull aPull, tbHalGpioValue aValue)
{
    NRF_GPIO_Type *gpio   = gpioGet(aPin);
    uint8_t        pin    = GPIO_GET_PIN(aPin);
    uint32_t       config = (((uint32_t)aDirection << GPIO_PIN_CNF_DIR_Pos) & GPIO_PIN_CNF_DIR_Msk) |
                      ((gpioGetPullFlag(aPull) << GPIO_PIN_CNF_PULL_Pos) & GPIO_PIN_CNF_PULL_Msk) |
                      (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);

    if ((gpio->PIN_CNF[pin] & GPIO_PIN_CNF_DIR_Msk) == GPIO_PIN_CNF_DIR_Output)
    {
        // Previous mode was an output, set cfg reg first to avoid glitching the output value
        // if the new mode isn't output.
        gpio->PIN_CNF[pin] = config;
        gpioSetOutput(gpio, pin, aValue);
    }
    else
    {
        // Previous mode was not an output, set out reg first so the value is
        // right before the new mode is set.
        // if the new mode is output, this is imgpioant to avoid glitching.
        // if the new mode is not output, it's doesn't really matter but
        // shouldn't hurt.
        gpioSetOutput(gpio, pin, aValue);
        gpio->PIN_CNF[pin] = config;
    }
}

bool tbHalGpioIsValid(tbHalGpioPin aPin)
{
    return gpioIsPinValid(aPin);
}

void tbHalGpioOutputRequest(tbHalGpioPin aPin, tbHalGpioValue aValue)
{
    gpioRequest(aPin, kGpioDirectionOutput, TB_HAL_GPIO_PULL_NONE, aValue);
}

void tbHalGpioInputRequest(tbHalGpioPin aPin, tbHalGpioPull aPull)
{
    // The output register OUT is set to 0 by default, so here state is set to LOW.
    gpioRequest(aPin, kGpioDirectionInput, aPull, TB_HAL_GPIO_VALUE_LOW);
}

void tbHalGpioRelease(tbHalGpioPin aPin)
{
    NRF_GPIO_Type *gpio = gpioGet(aPin);
    uint8_t        pin  = GPIO_GET_PIN(aPin);

    // Set the pin as an input and disconnect the input buffer.
    gpio->PIN_CNF[pin] =
        (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
}

void tbHalGpioSetValue(tbHalGpioPin aPin, tbHalGpioValue aValue)
{
    gpioSetOutput(gpioGet(aPin), GPIO_GET_PIN(aPin), aValue);
}

tbHalGpioValue tbHalGpioGetValue(tbHalGpioPin aPin)
{
    NRF_GPIO_Type *gpio = gpioGet(aPin);
    uint8_t        pin  = GPIO_GET_PIN(aPin);

    return (tbHalGpioValue)((gpio->IN >> pin) & 0x01);
}

otError tbHalGpioIntRequest(tbHalGpioPin         aPin,
                            tbHalGpioPull        aPull,
                            tbHalGpioIntMode     aMode,
                            tbHalGpioIntCallback aCallback)
{
    otError        error = OT_ERROR_NONE;
    NRF_GPIO_Type *gpio  = gpioGet(aPin);
    uint8_t        pin   = GPIO_GET_PIN(aPin);
    tbHalGpioValue state = (tbHalGpioValue)((gpio->IN >> pin) & 0x01);
    uint32_t       sense;
    IntEntry *     entry;

    otEXPECT_ACTION(aCallback != NULL, error = OT_ERROR_FAILED);
    gpio->PIN_CNF[pin] = ((GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) & GPIO_PIN_CNF_DIR_Msk) |
                         ((gpioGetPullFlag(aPull) << GPIO_PIN_CNF_PULL_Pos) & GPIO_PIN_CNF_PULL_Msk);

    // The GPIO rising/falling time is between 9ns and 25ns.
    // If the pin is in floating state before, after we set the pull mode, the
    // state of the pin may be changed. Here we wait the pin entering stable
    // state.
    tbHalDelayUs(1);

    // Get the current state of the pin.
    state = (tbHalGpioValue)((gpio->IN >> pin) & 0x01);

    // SENSE provides level detection only. if the current level is low, we
    // need to configure for SENSE_High.
    // If the current level is high, we need to configure for SENSE_Low.
    // Then when we get the interrupt, switch to opposite SENSE state.
    sense = (state == TB_HAL_GPIO_VALUE_HIGH) ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High;

    tbHalInterruptDisable();

    entry = gpioFindEmptyIntEntry();
    otEXPECT_ACTION(entry != NULL, error = OT_ERROR_FAILED);

    entry->mCallback = aCallback;
    entry->mIntPin   = aPin;
    entry->mIntMode  = aMode;

    // Set new value for SENSE.
    gpio->PIN_CNF[pin] = ((GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) & GPIO_PIN_CNF_DIR_Msk) |
                         ((gpioGetPullFlag(aPull) << GPIO_PIN_CNF_PULL_Pos) & GPIO_PIN_CNF_PULL_Msk) |
                         ((sense << GPIO_PIN_CNF_SENSE_Pos) & GPIO_PIN_CNF_SENSE_Msk);

exit:
    tbHalInterruptEnable();
    return error;
}

static void gpioUpdateSense(NRF_GPIO_Type *aGpio, uint8_t aPin, uint32_t aSense)
{
    // Disable PORT interrupt when we do the change to prevent spurious interrupts.
    NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;

    // Set new SENSE level.
    aGpio->PIN_CNF[aPin] &= ~GPIO_PIN_CNF_SENSE_Msk;
    aGpio->PIN_CNF[aPin] |= aSense << GPIO_PIN_CNF_SENSE_Pos;

    // Enable PORT interrupt.
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Set << GPIOTE_INTENSET_PORT_Pos;
}

static void gpioInterruptHandler(NRF_GPIO_Type *aGpio, uint8_t aPort)
{
    uint32_t latchValue     = aGpio->LATCH;
    uint32_t latchValueCopy = latchValue;

    while (latchValue)
    {
        uint32_t       pin      = __builtin_ffs(latchValue) - 1;
        tbHalGpioValue state    = (tbHalGpioValue)((aGpio->IN >> pin) & 0x01);
        uint32_t       oldSense = (aGpio->PIN_CNF[pin] & GPIO_PIN_CNF_SENSE_Msk) >> GPIO_PIN_CNF_SENSE_Pos;
        IntEntry *     entry    = gpioFindIntEntry(GPIO_PIN(aPort, pin));
        uint32_t       sense;

        assert(entry != NULL);
        assert(entry->mCallback != NULL);

        switch (entry->mIntMode)
        {
        case TB_HAL_GPIO_INT_MODE_RISING:
            if (oldSense == GPIO_PIN_CNF_SENSE_Low)
            {
                if (state == TB_HAL_GPIO_VALUE_HIGH)
                {
                    // The SENSE was configured for low, but current GPIO state
                    // is high. It means the state switched to low then to high.
                    // There must be a rising edge happened.
                    entry->mCallback(GPIO_PIN(aPort, pin));
                }
                else
                {
                    // The SENSE was configured for low, then initial state was
                    // high and we needed to wait for the low before we could
                    // switch to high.
                    gpioUpdateSense(aGpio, pin, GPIO_PIN_CNF_SENSE_High);
                }
            }
            else
            {
                // Check current gpio state, if it is low, then leave SENSE
                // config unchanged. if it is high, then we need to switch
                // SENSE config to low to restart rising edge detection.
                if (state == TB_HAL_GPIO_VALUE_HIGH)
                {
                    gpioUpdateSense(aGpio, pin, GPIO_PIN_CNF_SENSE_Low);
                }

                entry->mCallback(GPIO_PIN(aPort, pin));
            }
            break;

        case TB_HAL_GPIO_INT_MODE_FALLING:
            if (oldSense == GPIO_PIN_CNF_SENSE_High)
            {
                if (state == TB_HAL_GPIO_VALUE_LOW)
                {
                    // The SENSE was configured for high, but current GPIO state
                    // is low. It means the state switched to high then to low.
                    // There must be a falling edge happened.
                    entry->mCallback(GPIO_PIN(aPort, pin));
                }
                else
                {
                    // The SENSE was configured for high, then initial state was
                    // low and we needed to wait for the high before we could
                    // switch to low.
                    gpioUpdateSense(aGpio, pin, GPIO_PIN_CNF_SENSE_Low);
                }
            }
            else
            {
                // Check current gpio state, if it is high, then leave SENSE
                // config unchanged. if it is low, then we need to switch SENSE
                // config to high to restart falling edge detection.
                if (state == TB_HAL_GPIO_VALUE_LOW)
                {
                    gpioUpdateSense(aGpio, pin, GPIO_PIN_CNF_SENSE_High);
                }

                entry->mCallback(GPIO_PIN(aPort, pin));
            }
            break;

        case TB_HAL_GPIO_INT_MODE_BOTH_EDGES:
            // if we want interrupt on both edges, configure SENSE again so we
            // catch the other transition next. this can be tricky if the pin
            // signal is bouncing around. This is a best effort and not
            // guaranteed to catch signals that can toggle at high frequency.
            sense = (oldSense == GPIO_PIN_CNF_SENSE_High) ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High;

            gpioUpdateSense(aGpio, pin, sense);
            entry->mCallback(GPIO_PIN(aPort, pin));
            break;

        default:
            break;
        }

        // Clear shadow reg bit
        latchValue &= ~(1UL << pin);
    }

    // Clear previously detected LATCH bits with one write. If we clear one at
    // a time, there is a possibility of generating several new PORT events
    // with each clear when in LDETECT mode. This can cause an extra pending
    // PORT event while we loop through the set bits, and would cause us to
    // take an extra interrupt unnecessarily.
    if (latchValueCopy)
    {
        aGpio->LATCH = latchValueCopy;
    }
}

void tbHalGpioIntRelease(tbHalGpioPin aPin)
{
    NRF_GPIO_Type *gpio = gpioGet(aPin);
    uint8_t        pin  = GPIO_GET_PIN(aPin);

    tbHalInterruptDisable();

    // Clear the SENSE level and disconnect the input buffer.
    gpio->PIN_CNF[aPin] &= ~GPIO_PIN_CNF_SENSE_Msk;
    gpio->PIN_CNF[aPin] |= (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);

    // Clear LATCH bit just in case it was set
    gpio->LATCH = 1 << pin;

    gpioFreeIntEntry(aPin);
    tbHalInterruptEnable();
}

void GPIOTE_IRQHandler(void)
{
    // Clear PORT event.
    NRF_GPIOTE->EVENTS_PORT = 0;
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);

    for (uint8_t i = 0; i < GPIO_COUNT; i++)
    {
        gpioInterruptHandler(sGpios[i], i);
    }
}
