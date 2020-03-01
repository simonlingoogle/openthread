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
 *   This file includes the hardware abstraction layer for GPIO.
 *
 */

#ifndef CHIPS_INCLUDE_GPIO_H__
#define CHIPS_INCLUDE_GPIO_H__

#include <stdint.h>

#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type represents GPIO pin.
 *
 */
typedef uint8_t tbHalGpioPin;

/**
 * This enumeration defines the GPIO interrupt modes.
 *
 */
typedef enum
{
    TB_HAL_GPIO_INT_MODE_RISING     = 0, ///< External Interrupt Mode with Rising edge trigger detection.
    TB_HAL_GPIO_INT_MODE_FALLING    = 1, ///< External Interrupt Mode with Falling edge trigger detection.
    TB_HAL_GPIO_INT_MODE_BOTH_EDGES = 2, ///< External Interrupt Mode with Rising/Falling edge trigger detection.
} tbHalGpioIntMode;

/**
 * This enumeration defines the GPIO pulling modes.
 *
 */
typedef enum
{
    TB_HAL_GPIO_PULL_NONE     = 0, ///< No Pull-up or Pull-down activation.
    TB_HAL_GPIO_PULL_PULLDOWN = 1, ///< Pull-down activation.
    TB_HAL_GPIO_PULL_PULLUP   = 2, ///< Pull-up activation.
} tbHalGpioPull;

/**
 * This enumeration defines the GPIO pin state values.
 *
 */
typedef enum
{
    TB_HAL_GPIO_VALUE_LOW  = 0, ///< GPIO state low.
    TB_HAL_GPIO_VALUE_HIGH = 1, ///< GPIO state high.
} tbHalGpioValue;

/**
 * This method initializes the GPIO module.
 *
 */
void tbHalGpioInit(void);

/**
 * This function pointer is called when a GPIO interrupt is generated. It is
 * called from ISR context and it should be executed very quickly, to minimize
 * the chance that the ISR might be pre-empted by another interrupt.
 *
 * @param[in]  aPin  GPIO pin.
 *
 */
typedef void (*tbHalGpioIntCallback)(tbHalGpioPin aPin);

/**
 * This method indicates whether the given GPIO pin is valid pin.
 *
 * @param[in] aPin  GPIO pin.
 *
 * @retval  true if the given GPIO pin is valid, false otherwise.
 *
 */
bool tbHalGpioIsValid(tbHalGpioPin aPin);

/**
 * This method sets the given GPIO pin as an output pin.
 *
 * @param[in] aPin   GPIO pin.
 * @param[in] aValue GPIO state value.
 *
 */
void tbHalGpioOutputRequest(tbHalGpioPin aPin, tbHalGpioValue aValue);

/**
 * This method sets the given GPIO pin as an input pin.
 *
 * @param[in] aPin   GPIO pin.
 * @param[in] aPull  Pulling mode.
 *
 */
void tbHalGpioInputRequest(tbHalGpioPin aPin, tbHalGpioPull aPull);

/**
 * This method sets GPIO output value.
 *
 * @note: User must call the method `tbHalGpioOutputRequest()` before calling this method.
 *
 * @param[in] aPin    GPIO pin.
 * @param[in] aValue  GPIO output state value.
 *
 */
void tbHalGpioSetValue(tbHalGpioPin aPin, tbHalGpioValue aValue);

/**
 * This method gets GPIO current state value.
 *
 * @note: User must call the method `tbHalGpioOutputRequest()`, `tbHalGpioInputRequest()`
 *        or `tbHalGpioIntRequest()` before calling this method.
 *
 * @return GPIO input state value.
 *
 */
tbHalGpioValue tbHalGpioGetValue(tbHalGpioPin aPin);

/**
 * This method uninitializes a GPIO input/output pin to make it enter low power mode.
 *
 * @param[in] pin GPIO pin.
 *
 */
void tbHalGpioRelease(tbHalGpioPin aPin);

/**
 * This method sets the given GPIO pin as an input interrupt pin.
 *
 * @param[in] aPin       GPIO pin.
 * @param[in] aPull      Pulling mode of GPIO pin.
 * @param[in] aMode      GPIO interrupt mode.
 * @param[in] aCallback  A pointer to a function that will be called when the interrupt is happened.
 *
 * @retval OT_ERROR_NONE    Initialization was successful.
 * @retval OT_ERROR_FAILED  Failed to initialize an interrupt pin.
 *
 */
otError tbHalGpioIntRequest(tbHalGpioPin         aPin,
                            tbHalGpioPull        aPull,
                            tbHalGpioIntMode     aMode,
                            tbHalGpioIntCallback aCallback);

/**
 * This method uninitializes a GPIO interrupt pin to make it enter low power mode.
 *
 * @param[in] aPin GPIO pin.
 *
 */
void tbHalGpioIntRelease(tbHalGpioPin aPin);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_GPIO_H__
