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

/**
 * @file
 *   This file includes definitions for power manager.
 */

#ifndef PLATFORM_POWER_MANAGER_H__
#define PLATFORM_POWER_MANAGER_H__

#include <stdint.h>
#include <openthread/error.h>

#include "platform/power_settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function returns the power setting for the given given regulatory code, channel and target power.
 *
 * @param[in]   aRegulatoryCode  The regulatory code.
 * @param[in]   aChannel         The channel.
 * @param[in]   aTargetPower     The Target power, in 0.01 dBm.
 * @param[out]  aPowerSetting    A pointer to power setting structure.
 *
 * @retval OT_ERROR_NONE          Successfully retrived the power setting.
 * @retval OT_ERROR_INVALID_ARGS  @p aRegulatoryCode is invalid or @p aPowerSetting is NULL.
 * @retval OT_ERROR_NOT_FOUND     No power setting was found.
 *
 */
otError tbPowerManagerGetPowerSetting(uint16_t        aRegulatoryCode,
                                      uint8_t         aChannel,
                                      int16_t         aTargetPower,
                                      tbPowerSetting *aPowerSetting);

#ifdef __cplusplus
} // extern "C"
#endif
#endif // PLATFORM_POWER_MANAGER_H__
