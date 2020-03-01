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
 *   This file configures the stack size, stack alignement and heap size.
 *
 */

#ifndef STARTUP_CONFIG_H_
#define STARTUP_CONFIG_H_

/**
 * @def __STARTUP_CONFIG_STACK_SIZE
 *
 * Define size of stack. Size must be multiple of 4.
 *
 */
#define __STARTUP_CONFIG_STACK_SIZE TERBIUM_MEMORY_MIN_STACK_SIZE

/**
 * @def __STARTUP_CONFIG_STACK_ALIGNEMENT
 *
 * Define alignement of stack. Alignment will be 2 to the power of __STARTUP_CONFIG_STACK_ALIGNEMENT.
 * Since calling convention requires that the stack is aligned to 8-bytes when a function is called,
 * the minimum __STARTUP_CONFIG_STACK_ALIGNEMENT is therefore 3.
 *
 */
#define __STARTUP_CONFIG_STACK_ALIGNEMENT 3

/**
 * @def __STARTUP_CONFIG_HEAP_SIZE
 *
 * Define size of heap. Size must be multiple of 4.
 *
 */
#define __STARTUP_CONFIG_HEAP_SIZE TERBIUM_MEMORY_HEAP_SIZE

#endif // STARTUP_CONFIG_H_
