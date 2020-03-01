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
 *   This file includes definitions of sysenv keys.
 *
 */

#ifndef PLATFORM_SYSENV_KEYS_H__
#define PLATFORM_SYSENV_KEYS_H__

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
#define TB_SYSENV_KEY_802154_ADDRESS        "hwaddr0"
#define TB_SYSENV_KEY_REGULATORY_DOMAIN     "nlwirelessregdom"
#define TB_SYSENV_KEY_WIRELESS_CAL          "nlwirelessregcal"
#define TB_SYSENV_WIRELESS_CAL_IDENTIFIER   "6LoWPAN"
#define TB_SYSENV_POWER_SETTINGS_IDENTIFIER "txPowLookup"
// clang-format on

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_SYSENV_KEYS_H__
