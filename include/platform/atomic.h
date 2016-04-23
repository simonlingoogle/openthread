/*
 *    Copyright 2016 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *    @brief
 *      Defines the interface to the Atomic object that implements
 *      critical sections within the Thread stack.
 */

#ifndef ATOMIC_HPP_
#define ATOMIC_HPP_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup atomic Atomic
 * @ingroup platform
 *
 * @brief
 *   This module includes the platform abstraction to support critical sections.
 *
 * @{
 *
 */

/**
 * Begin critical section.
 */
uint32_t ot_atomic_begin();

/**
 * End critical section.
 */
void ot_atomic_end(uint32_t state);

/**
 * @}
 *
 */

#ifdef __cplusplus
}  // end of extern "C"
#endif

#endif  // ATOMIC_HPP_
