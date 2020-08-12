/*
 *
 *    Copyright (c) 2020 Google, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Google. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Google.
 *
 */

#ifndef OT_LEGACY_PAIRING_EXT_H_
#define OT_LEGACY_PAIRING_EXT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method initializes the legacy processing.
 *
 */
void otLegacyInit(void);

/**
 * This method starts the legacy processing.
 *
 * Will be called when Thread network is enabled.
 *
 */
void otLegacyStart(void);

/**
 * This method stops the legacy processing.
 *
 * Will be called when Thread network is disabled.
 *
 */
void otLegacyStop(void);

/**
 * Defines handler (function pointer) type for setting the legacy ULA prefix.
 *
 * @param[in] aUlaPrefix   A pointer to buffer containing the legacy ULA prefix.
 *
 * Invoked to set the legacy ULA prefix.
 *
 */
void otSetLegacyUlaPrefix(const uint8_t *aUlaPrefix);

#ifdef __cplusplus
}
#endif

#endif // OT_LEGACY_PAIRING_EXT_H_
