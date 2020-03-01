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
 *   This file implements the hardware abstraction layer for I2C.
 *
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <utils/code_utils.h>

#include <nrfx/mdk/nrf.h>

#if defined(NRF52840_XXAA)
#include <nrfx/mdk/nrf52840_peripherals.h>
#elif defined(NRF52811_XXAA)
#include <nrfx/mdk/nrf52811_peripherals.h>
#endif

#include "chips/include/gpio.h"
#include "chips/include/i2c.h"

#define NRF_TWIM_IRQ_PRIORITY 6
#define NRF_TWIM_CLEAR_ALL_MASK (0xFFFFFFFF)
#define NRF_TWIM_TX_BUFFER_SIZE 32

#if defined(NRF52840_XXAA)
#include <nrfx/mdk/nrf52840_peripherals.h>
static IRQn_Type      sI2cIrqs[TWIM_COUNT] = {SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn,
                                         SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQn};
static NRF_TWIM_Type *sI2cs[TWIM_COUNT]    = {NRF_TWIM0, NRF_TWIM1};
#elif defined(NRF52811_XXAA)
#include <nrfx/mdk/nrf52811_peripherals.h>
static IRQn_Type      sI2cIrqs[TWIM_COUNT] = {TWIM0_TWIS0_TWI0_SPIM1_SPIS1_SPI1_IRQn};
static NRF_TWIM_Type *sI2cs[TWIM_COUNT]    = {NRF_TWIM0};
#endif

static uint8_t                     sTxBuffer[TWIM_COUNT][NRF_TWIM_TX_BUFFER_SIZE];
static tbHalI2cRegisterAddressSize sI2cRegisterAddressSizes[TWIM_COUNT];
static bool                        sI2sInitilaized[TWIM_COUNT];

static volatile uint32_t sI2cError;
static volatile bool     sI2cTransferCompleted;

static inline NRF_TWIM_Type *i2cGet(tbHalI2cId aI2cId)
{
    assert(aI2cId < otARRAY_LENGTH(sI2cs));
    return sI2cs[aI2cId];
}

static inline bool isBufferInDataRam(void const *const aBuffer, uint16_t aLength)
{
    uint32_t start = (uint32_t)aBuffer;
    uint32_t end   = (uint32_t)((uint8_t *)aBuffer + aLength);

    return (start >= TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS) && (end < TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS);
}

static uint32_t i2cFrequencyToReg(tbHalI2cFrequency aFrequency)
{
    // This table refers to the TWIM section of nrf52xxx datasheet.
    const struct
    {
        uint32_t          mFrequencyReg;
        tbHalI2cFrequency mFrequency;
    } frequencyTable[] = {
        {0x01980000, TB_HAL_I2C_FREQ_100K}, {0x04000000, TB_HAL_I2C_FREQ_250K}, {0x06400000, TB_HAL_I2C_FREQ_400K}};

    uint32_t frequencyReg = 0;

    for (size_t i = 0; i < otARRAY_LENGTH(frequencyTable); i++)
    {
        if (frequencyTable[i].mFrequency == aFrequency)
        {
            frequencyReg = frequencyTable[i].mFrequencyReg;
            break;
        }
    }

    assert(frequencyReg != 0);

    return frequencyReg;
}

otError tbHalI2cInit(tbHalI2cId aI2cId, const tbHalI2cConfig *aConfig)
{
    otError        error = OT_ERROR_NONE;
    NRF_TWIM_Type *i2c   = i2cGet(aI2cId);
    uint32_t       frequencyReg;

    otEXPECT_ACTION(aConfig != NULL, error = OT_ERROR_INVALID_ARGS);

    // Nrf528xx only supports 7 bits slave address.
    otEXPECT_ACTION(aConfig->mSlaveAddressSize == TB_HAL_I2C_SLAVE_ADDRESS_SIZE_7_BITS, error = OT_ERROR_INVALID_ARGS);

    otEXPECT_ACTION((frequencyReg = i2cFrequencyToReg(aConfig->mFrequency)) != 0, error = OT_ERROR_INVALID_ARGS);

    // Configured as an input per nrf52xxx spec.
    tbHalGpioInputRequest(aConfig->mPinSda, TB_HAL_GPIO_PULL_NONE);
    tbHalGpioInputRequest(aConfig->mPinScl, TB_HAL_GPIO_PULL_NONE);

    // Route controller to pins.
    i2c->PSEL.SDA = aConfig->mPinSda;
    i2c->PSEL.SCL = aConfig->mPinScl;

    // Set I2c clock frequency.
    i2c->FREQUENCY = frequencyReg;

    // Set slave device address.
    i2c->ADDRESS = aConfig->mSlaveAddress;

    sI2cRegisterAddressSizes[aI2cId] = aConfig->mRegisterAddressSize;
    sI2sInitilaized[aI2cId]          = true;

exit:
    return error;
}

void tbHalI2cDeinit(tbHalI2cId aI2cId)
{
    NRF_TWIM_Type *i2c = i2cGet(aI2cId);

    // Disconnect I2C pins.
    i2c->PSEL.SDA = 0xffffffff;
    i2c->PSEL.SCL = 0xffffffff;

    sI2sInitilaized[aI2cId] = false;
}

static void i2cStartWrite(tbHalI2cId aI2cId, uint8_t *aBuffer, uint16_t aLength, uint8_t aRegisterAddressSize)
{
    NRF_TWIM_Type *i2c = i2cGet(aI2cId);

    if (aRegisterAddressSize || !isBufferInDataRam(aBuffer, aLength))
    {
        // EasyDMA is a module implemented by some peripherals to gain direct access to Data RAM.
        // If TXD.PTR is not pointing to a valid data RAM region, an EasyDMA transfer may result
        // in a HardFault or RAM corruption.
        // For I2C writes, NRF52x DMA/TWI hardware does not support switching of buffers
        // between register address and data without a repeated start in between. Therefore
        // we copy both into a TX buffer first in order to send in one shot.
        assert(aRegisterAddressSize + aLength <= NRF_TWIM_TX_BUFFER_SIZE);
        memcpy(&sTxBuffer[aI2cId][aRegisterAddressSize], aBuffer, aLength);
        aLength += aRegisterAddressSize;
        i2c->TXD.PTR = (uint32_t)&sTxBuffer[aI2cId][0];
    }
    else
    {
        i2c->TXD.PTR = (uint32_t)aBuffer;
    }

    i2c->TXD.MAXCNT    = aLength;
    i2c->SHORTS        = TWIM_SHORTS_LASTTX_STOP_Msk;
    i2c->TASKS_STARTTX = 1;
}

static void i2cStartRead(tbHalI2cId aI2cId, uint8_t *aBuffer, uint16_t aLength, uint8_t aRegisterAddressSize)
{
    NRF_TWIM_Type *i2c = i2cGet(aI2cId);

    i2c->RXD.PTR    = (uint32_t)aBuffer;
    i2c->RXD.MAXCNT = aLength;

    if (aRegisterAddressSize)
    {
        // Write the register address to slave device, then start to read data from given register address.
        i2c->TXD.PTR       = (uint32_t)&sTxBuffer[aI2cId][0];
        i2c->TXD.MAXCNT    = aRegisterAddressSize;
        i2c->SHORTS        = TWIM_SHORTS_LASTTX_STARTRX_Msk | TWIM_SHORTS_LASTRX_STOP_Msk;
        i2c->TASKS_STARTTX = 1;
    }
    else
    {
        // Directly read data from slave device.
        i2c->SHORTS        = TWIM_SHORTS_LASTRX_STOP_Msk;
        i2c->TASKS_STARTRX = 1;
    }
}

static otError i2cTransfer(tbHalI2cId aI2cId, uint16_t aAddress, uint8_t *aBuffer, uint16_t aLength, bool aWrite)
{
    otError        error               = OT_ERROR_NONE;
    NRF_TWIM_Type *i2c                 = i2cGet(aI2cId);
    uint8_t *      addrBytes           = (uint8_t *)&aAddress;
    uint8_t        registerAddressSize = sI2cRegisterAddressSizes[aI2cId];

    otEXPECT_ACTION(sI2sInitilaized[aI2cId], error = OT_ERROR_FAILED);
    otEXPECT_ACTION((aBuffer != NULL) && (aLength != 0), error = OT_ERROR_INVALID_ARGS);

    if (registerAddressSize == TB_HAL_I2C_REGISTER_ADDRESS_SIZE_2_BYTE)
    {
        // Reverse byte-order of the register address such that DMA sends it MSB first.
        sTxBuffer[aI2cId][0] = addrBytes[1];
        sTxBuffer[aI2cId][1] = addrBytes[0];
    }
    else if (registerAddressSize == TB_HAL_I2C_REGISTER_ADDRESS_SIZE_1_BYTE)
    {
        sTxBuffer[aI2cId][0] = addrBytes[0];
    }

    // Clear any shortcuts, events, and errors.
    i2c->SHORTS         = 0;
    i2c->ERRORSRC       = NRF_TWIM_CLEAR_ALL_MASK;
    i2c->EVENTS_ERROR   = 0;
    i2c->EVENTS_STOPPED = 0;

    // Configure and start.
    i2c->INTENSET = TWIM_INTENSET_STOPPED_Msk | TWIM_INTENSET_ERROR_Msk;
    i2c->ENABLE   = TWIM_ENABLE_ENABLE_Enabled << TWIM_ENABLE_ENABLE_Pos;

    // Enable NVIC interrupt.
    NVIC_SetPriority(sI2cIrqs[aI2cId], NRF_TWIM_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(sI2cIrqs[aI2cId]);
    NVIC_EnableIRQ(sI2cIrqs[aI2cId]);

    if (aWrite)
    {
        i2cStartWrite(aI2cId, aBuffer, aLength, registerAddressSize);
    }
    else
    {
        i2cStartRead(aI2cId, aBuffer, aLength, registerAddressSize);
    }

    sI2cError             = 0;
    sI2cTransferCompleted = false;

    // Waiting for the transfer to be completed.
    while ((!sI2cTransferCompleted) && (sI2cError == 0))
        ;

    // Disable I2C.
    i2c->ENABLE = TWIM_ENABLE_ENABLE_Disabled << TWIM_ENABLE_ENABLE_Pos;

    // Disable NVIC interrupt.
    NVIC_DisableIRQ(sI2cIrqs[aI2cId]);
    NVIC_ClearPendingIRQ(sI2cIrqs[aI2cId]);

    error = (sI2cError != 0) ? OT_ERROR_FAILED : OT_ERROR_NONE;

exit:
    return error;
}

otError tbHalI2cWrite(tbHalI2cId aI2cId, uint16_t aAddress, uint8_t *aBuffer, uint16_t aLength)
{
    return i2cTransfer(aI2cId, aAddress, aBuffer, aLength, true);
}

otError tbHalI2cRead(tbHalI2cId aI2cId, uint16_t aAddress, uint8_t *aBuffer, uint16_t aLength)
{
    // EasyDMA is a module implemented by some peripherals to gain direct access to Data RAM.
    // If TXD.PTR is not pointing to a valid data RAM region, an EasyDMA transfer may result in
    // a HardFault or RAM corruption.
    assert(isBufferInDataRam(aBuffer, aLength));
    return i2cTransfer(aI2cId, aAddress, aBuffer, aLength, false);
}

static void i2cIrqHandler(tbHalI2cId aI2cId)
{
    NRF_TWIM_Type *i2c  = i2cGet(aI2cId);
    uint32_t       irqs = i2c->INTENSET;

    if ((irqs & TWIM_INTENSET_ERROR_Msk) && i2c->EVENTS_ERROR)
    {
        // Clean ERROR event flag.
        i2c->EVENTS_ERROR = 0;

        // Get error source.
        sI2cError = i2c->ERRORSRC;

        // Triger STOP task.
        i2c->TASKS_STOP = 1;
    }

    if ((irqs & TWIM_INTENSET_STOPPED_Msk) && i2c->EVENTS_STOPPED)
    {
        // Clean STOPPED event flag.
        i2c->EVENTS_STOPPED = 0;

        // Disable all interrupts.
        i2c->INTEN = 0;

        sI2cTransferCompleted = true;
    }
}

#if defined(NRF52840_XXAA)
void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void)
{
    i2cIrqHandler(0);
}

void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void)
{
    i2cIrqHandler(1);
}
#elif defined(NRF52811_XXAA)

void TWIM0_TWIS0_TWI0_SPIM1_SPIS1_SPI1_IRQHandler(void)
{
    i2cIrqHandler(0);
}
#endif
