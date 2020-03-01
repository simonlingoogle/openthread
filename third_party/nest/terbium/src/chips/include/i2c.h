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
 *   This file includes the hardware abstraction layer for I2C.
 *
 */

#ifndef CHIPS_INCLUDE_I2C_H__
#define CHIPS_INCLUDE_I2C_H__

#include <stdint.h>

#include <openthread/error.h>
#include "chips/include/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type represents I2C identifier.
 *
 */
typedef uint8_t tbHalI2cId;

/**
 * This enumeration defines the I2C clock frequences.
 *
 */
typedef enum
{
    TB_HAL_I2C_FREQ_100K = 100000,
    TB_HAL_I2C_FREQ_250K = 250000,
    TB_HAL_I2C_FREQ_400K = 400000,
} tbHalI2cFrequency;

/**
 * This enumeration defines the size of I2C register address.
 *
 */
typedef enum
{
    TB_HAL_I2C_REGISTER_ADDRESS_SIZE_0_BYTE = 0,
    TB_HAL_I2C_REGISTER_ADDRESS_SIZE_1_BYTE = 1,
    TB_HAL_I2C_REGISTER_ADDRESS_SIZE_2_BYTE = 2,
} tbHalI2cRegisterAddressSize;

/**
 * This enumeration defines the size of the I2C slave address.
 *
 */
typedef enum
{
    TB_HAL_I2C_SLAVE_ADDRESS_SIZE_7_BITS  = 0,
    TB_HAL_I2C_SLAVE_ADDRESS_SIZE_10_BITS = 1,
} tbHalI2cSlaveAddressSize;

/**
 * This structure defines the I2c configurations.
 *
 */
typedef struct tbHalI2cConfig
{
    tbHalGpioPin                mPinSda;              ///< I2C SDA pin.
    tbHalGpioPin                mPinScl;              ///< I2C SCL pin.
    tbHalI2cFrequency           mFrequency;           ///< I2C clock frequency.
    tbHalI2cRegisterAddressSize mRegisterAddressSize; ///< Size of register address.
    tbHalI2cSlaveAddressSize    mSlaveAddressSize;    ///< Size of I2C slave device address.
    uint16_t                    mSlaveAddress;        ///< I2C slave device addesss.
} tbHalI2cConfig;

/**
 * This method initializes the I2C.
 *
 * @param[in] aI2cId   I2c identifier.
 * @param[in] aConfig  A pointer to the I2c configuration structure.
 *
 * @retval OT_ERROR_NONE          Initialization was successful.
 * @retval OT_ERROR_INVALID_ARGS  The @p aConfig is NULL or the configuration is not supported by I2c driver.
 */
otError tbHalI2cInit(tbHalI2cId aI2cId, const tbHalI2cConfig *aConfig);

/**
 * This method de-initializes the I2C.
 *
 * @param[in] aI2cId  I2C identifier.
 *
 */
void tbHalI2cDeinit(tbHalI2cId aI2cId);

/**
 * This method writes one or more bytes of data to the given I2C device at the given register address.
 * Blocks until a transfer has completed and will return an error value and close the bus transaction if any
 * errors are encountered.
 *
 * @param[in] aI2cId    I2C identifier.
 * @param[in] aAddress  The address of the data to be written.
 * @param[in] aBuffer   A pointer to a data buffer which will be written to I2C bus.
 * @param[in] aLength   The length of the buffer.
 *
 * @retval OT_ERROR_NONE          Successfully written the buffer to I2C bus.
 * @retval OT_ERROR_INVALID_ARGS  The @p aBuffer was NULL or @p aLength was 0.
 * @retval OT_ERROR_FAILED        Failed to write the buffer to I2C bus.
 *
 */
otError tbHalI2cWrite(tbHalI2cId aI2cId, uint16_t aAddress, uint8_t *aBuffer, uint16_t aLength);

/**
 * This method reads one or more bytes of data from the given I2C device from the given register address.
 * Blocks until a transfer has completed and will return an error value and close the bus transaction if any
 * errors are encountered.
 *
 * @param[in] aI2cId    I2C identifier.
 * @param[in] aAddress  The address of the data to be read.
 * @param[in] aBuffer   A pointer to a data buffer which will store the read data from I2C bus.
 * @param[in] aLength   The length of the buffer.
 *
 * @retval OT_ERROR_NONE          Successfully read the data from I2C bus.
 * @retval OT_ERROR_INVALID_ARGS  The @p aBuffer was NULL or @p aLength was 0.
 * @retval OT_ERROR_FAILED        Failed to read data from I2C bus.
 */
otError tbHalI2cRead(tbHalI2cId aI2cId, uint16_t aAddress, uint8_t *aBuffer, uint16_t aLength);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_I2C_H__
