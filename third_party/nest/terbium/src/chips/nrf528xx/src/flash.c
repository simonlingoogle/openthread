/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>

#include <utils/code_utils.h>

#include "platform-nrf5.h"

#define FLASH_PAGE_ADDR_MASK 0xFFFFF000
#define FLASH_PAGE_SIZE 4096

uint32_t tbHalFlashRead(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    otEXPECT(aData);
    otEXPECT(aAddress + aSize <= TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS);

    memcpy(aData, (uint8_t *)aAddress, aSize);
    ret = aSize;

exit:
    return ret;
}

uint32_t tbHalFlashWrite(uint32_t aAddress, uint8_t *aData, uint32_t aSize)
{
    uint32_t ret = 0;

    otEXPECT(aData);
    otEXPECT(aAddress + aSize <= TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS);
    otEXPECT(aSize);

    otEXPECT(nrf5FlashWrite(aAddress, aData, aSize) == OT_ERROR_NONE);

    ret = aSize;

exit:
    return ret;
}

otError tbHalFlashErasePage(uint32_t aAddress, uint32_t aSize)
{
    otError error = OT_ERROR_NONE;

    otEXPECT_ACTION(aAddress + aSize <= TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS, error = OT_ERROR_INVALID_ARGS);
    // Address must align on the FLASH_PAGE_SIZE boundary.
    otEXPECT_ACTION((aAddress & (FLASH_PAGE_SIZE - 1)) == 0, error = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION((aSize & (FLASH_PAGE_SIZE - 1)) == 0, error = OT_ERROR_INVALID_ARGS);

    while (aSize)
    {
        otEXPECT((error = nrf5FlashPageErase(aAddress)) == OT_ERROR_NONE);
        aAddress += FLASH_PAGE_SIZE;
        aSize -= FLASH_PAGE_SIZE;
    }

exit:
    return error;
}
