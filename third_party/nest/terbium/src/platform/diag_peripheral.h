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
 *   This file includes definitions for processing peripheral diag commands.
 *
 */
#ifndef PLATFORM_DIAG_PERIPHERAL_H__
#define PLATFORM_DIAG_PERIPHERAL_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function processes peripheral diag commands.
 *
 * @param[in] aInstance    The OpenThread instance structure.
 * @param[in] aArgsLength  The number of elements in @p aArgs.
 * @param[in] aArgs        An array of arguments.
 *
 * @retval  OT_ERROR_NONE            The command was not processed successfully
 * @retval  OT_ERROR_INVALID_COMMAND The command is not supported.
 * @retval  OT_ERROR_INVALID_ARGS    The command is supported but invalid arguments provided.
 *
 */
otError tbDiagProcessPeripheral(otInstance *aInstance, uint8_t aArgsLength, char *aArgs[]);

/**
 * This function processes the diag alarm event.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 *
 */
void tbDiagAlarmCallback(otInstance *aInstance);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_DIAG_PERIPHERAL_H__
