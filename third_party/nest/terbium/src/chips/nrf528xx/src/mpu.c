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
 *   This file implements the hardware abstraction layer for Memory Protect Unit.
 *
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <nrfx/mdk/nrf.h>
#include "chips/include/cmsis.h"

// Same check used in CMSIS header.
#if (__MPU_PRESENT == 1)

// CMSIS doesn't define these PASR parameters.
// Refer section 9.3 of http://infocenter.arm.com/help/topic/com.arm.doc.ddi0337e/DDI0337E_cortex_m3_r1p1_trm.pdf
// to get these definitions.
#define MPU_RASR_AP_NO_ACCESS 0 ///< All accesses generate a permission fault.
#define MPU_RASR_AP_READ_ONLY 7 ///< Privileged/user read only.
#define MPU_RASR_TEX_NORMAL 1   ///< Outer and inner noncacheable.

#define ADDRESS_ZERO_GUARD_SIZE 256 ///< The address zero guard region size.

// Privileged and unprivileged no access normal memory, outer and inner non-cacheable, not shared.
#define MPU_ATTRIBUTE_NO_ACCESS \
    (MPU_RASR_XN_Msk | MPU_RASR_AP_NO_ACCESS << MPU_RASR_AP_Pos | MPU_RASR_TEX_NORMAL << MPU_RASR_TEX_Pos)

// A mask of all the reserved bits, to validate the aAttributes passed to mpuRequestRegion().
// None of these bits should be set in the aAttributes value.
#define MPU_RASR_ATTRIBUTES_Msk                                                                                \
    (MPU_RASR_XN_Msk | MPU_RASR_AP_Msk | MPU_RASR_TEX_Msk | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk | \
     MPU_RASR_SRD_Msk)

static int sZeroGuardRegion;
static int sStackGuardRegion;

static uint32_t mpuGetNumRegions(void)
{
    return (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
}

static void mpuInit(void)
{
    uint32_t i;
    uint32_t numRegions = mpuGetNumRegions();

    tbHalInterruptDisable();

    // Start by making sure MPU is off and all regions disabled.
    MPU->CTRL = 0;

    for (i = 0; i < numRegions; i++)
    {
        MPU->RBAR = MPU_RBAR_VALID_Msk | i;
        MPU->RASR = 0;
    }

    tbHalInterruptEnable();
}

static void mpuEnable(bool aEnable, bool aEnableDefaultMemoryMap, bool aEnableMpuInFaultHandlers)
{
    uint32_t value = 0;

    if (aEnable)
    {
        value = MPU_CTRL_ENABLE_Msk;

        if (aEnableDefaultMemoryMap)
        {
            value |= MPU_CTRL_PRIVDEFENA_Msk;
        }

        if (aEnableMpuInFaultHandlers)
        {
            value |= MPU_CTRL_HFNMIENA_Msk;
        }
    }

    MPU->CTRL = value;
}

static bool mpuIsMemoryAddressValid(int64_t aAddress)
{
    // When the value of TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS is 0, compiler generates an
    // warning "comparison of unsigned expression >= 0 is always true". To avoid this warning,
    // the type of the parameter is set to `int64_t`.
    return ((TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS <= aAddress) && (aAddress < TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS)) ||
           ((TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS <= aAddress) &&
            (aAddress < TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS));
}

static int32_t mpuRequestRegion(uint32_t aRegionBaseAddress, uint32_t aRegionSize, uint32_t aAttributes)
{
    int32_t  ret = -1;
    uint32_t i;
    uint32_t numRegions       = mpuGetNumRegions();
    uint64_t regionEndAddress = aRegionBaseAddress + aRegionSize;
    uint32_t sizeValue;

    assert((aRegionBaseAddress & ~MPU_RBAR_ADDR_Msk) == 0); // Base address must be 32-byte aligned.
    assert((aRegionSize & (aRegionSize - 1)) == 0);         // For cortex-m3 MPU, region size must be a power of 2.
    assert(aRegionSize >= 32);                              // The minimum region size is 32-bytes.
    assert((aAttributes & ~MPU_RASR_ATTRIBUTES_Msk) == 0);  // Only attribute bits should be set.
    assert(mpuIsMemoryAddressValid(aRegionBaseAddress) && mpuIsMemoryAddressValid(regionEndAddress));

    // Convert to what the RBAR wants for SIZE field.
    sizeValue = __builtin_ffs(aRegionSize) - 2;

    tbHalInterruptDisable();

    // Look for an unused region.
    for (i = 0; i < numRegions; i++)
    {
        // Region number to be configured.
        MPU->RNR = i;

        if ((MPU->RBAR == i) && (MPU->RASR == 0))
        {
            // Found one, write the RBAR register with the base address.
            MPU->RBAR = aRegionBaseAddress;

            // Write the aAttributes to RASR.
            MPU->RASR = aAttributes | (sizeValue << MPU_RASR_SIZE_Pos) | MPU_RASR_ENABLE_Msk;

            ret = i;
            break;
        }
    }

    tbHalInterruptEnable();

    return ret;
}

static void mpuReleaseRegion(int32_t aRegion)
{
    if (aRegion >= 0)
    {
        tbHalInterruptDisable();

        // Disables and releases a previously requested region.
        MPU->RBAR = MPU_RBAR_VALID_Msk | aRegion;
        MPU->RASR = 0;

        tbHalInterruptEnable();
    }
}

void nrf5MpuInit(void)
{
    extern uint32_t __STACKGUARD_start;

    mpuInit();

    // Set the address 0 guard to no access to catch NULL pointer usage.
    sZeroGuardRegion = mpuRequestRegion(0x00, ADDRESS_ZERO_GUARD_SIZE, MPU_ATTRIBUTE_NO_ACCESS);

    // Set the stack guard to no access to catch stack overflow fault.
    sStackGuardRegion =
        mpuRequestRegion((uint32_t)&__STACKGUARD_start, TERBIUM_MEMORY_STACK_GUARD_SIZE, MPU_ATTRIBUTE_NO_ACCESS);

    mpuEnable(true, true, true);
}

void nrf5MpuDeinit(void)
{
    mpuReleaseRegion(sZeroGuardRegion);
    mpuReleaseRegion(sStackGuardRegion);
}
#endif // (__MPU_PRESENT == 1)
