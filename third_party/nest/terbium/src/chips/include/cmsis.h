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
 *   This file includes the hardware abstraction layer for cmsis.
 *
 */

#ifndef CHIPS_INCLUDE_CMSIS_H__
#define CHIPS_INCLUDE_CMSIS_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function disables IRQ Interrupt.
 *
 */
void tbHalInterruptEnable(void);

/**
 * This function enables IRQ Interrupt.
 *
 */
void tbHalInterruptDisable(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CHIPS_INCLUDE_CMSIS_H__
