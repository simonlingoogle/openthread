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
 *   This file includes the hardware abstraction layer for flash.
 *
 */

#ifndef CHIPS_INCLUDE_FLASH_H__
#define CHIPS_INCLUDE_FLASH_H__

#include <stdint.h>
#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function reads @p aSize bytes from address @p aAddress into @p aData.
 *
 * @param[in]  aAddress  Flash address.
 * @param[out] aData     A pointer to the data buffer for reading.
 * @param[in]  aSize     Number of bytes to read.
 *
 * @returns The number of bytes read.
 *
 */
uint32_t tbHalFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize);

/**
 * This function writes @p aSize bytes from @p aData to address @p aAddress.
 *
 * @param[in]  aAddress  Flash address.
 * @param[out] aData     A pointer to the data to write.
 * @param[in]  aSize     Number of bytes to write.
 *
 * @returns The number of bytes written.
 *
 */
uint32_t tbHalFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize);

/**
 * This function erases the @p aSize bytes from address @p aAddress.
 *
 * @param[in]  aAddress  Flash address.
 * @param[in]  aSize     Number of bytes to earse.
 *
 * @retval OT_ERROR_NONE          Successfully earsed given flash area.
 * @retval OT_ERROR_INVALID_ARGS  The given flash area execeeds the real flash range or
 *                                the @p aAddress or @p aSize is not align with the flash page size.
 *
 */
otError tbHalFlashErasePage(uint32_t aAddress, uint32_t aSize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_FLASH_H__
