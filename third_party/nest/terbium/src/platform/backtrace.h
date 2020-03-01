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

/*
 * @file
 *   This file includes definitions for backtrace.
 *
 */

#ifndef PLATFORM_BACKTRACE_H__
#define PLATFORM_BACKTRACE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This method returns a backtrace for the calling program.
 *
 * @param[out] aBuffer  A pointer to where the backtrace should be written.
 * @param[in]  aLength  The maximum number of addresses that can be stored in buffer.
 *
 * @returns The level of the backtrace.
 *
 */
uint16_t tbBacktrace(uint32_t *aBuffer, uint16_t aLength);

/**
 * This method looks in the stack for a possible backtrace.
 *
 * @param[out] aStackPointer  The stack bottom address.
 * @param[in]  aStackTop      The stack top address.
 * @param[out] aBuffer        A pointer to where the backtrace should be written.
 * @param[in]  aLength        The maximum number of addresses that can be stored in buffer.
 * @param[in]  aMinLevel      The minimum level of the backtrace.
 * @param[in]  aMaxAttempts   The number of attempts to parse the backtrace.
 *
 * @returns The level of the backtrace.
 *
 */
uint16_t tbBacktraceNoContext(uint32_t  aStackPointer,
                              uint32_t  aStackTop,
                              uint32_t *aBuffer,
                              uint16_t  aLength,
                              uint16_t  aMinLevel,
                              uint16_t  aMaxAttempts);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_BACKTRACE_H__
