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
 *   This file includes project compile-time configuration constants
 *   for OpenThread.
 */

#ifndef OPENTHREAD_CORE_PROJECT_CONFIG_H_
#define OPENTHREAD_CORE_PROJECT_CONFIG_H_

/*
 * The GNU Autoconf system defines a PACKAGE macro which is the name
 * of the software package. This name collides with PACKAGE field in
 * the nRF52 Factory Information Configuration Registers (FICR)
 * structure.
 */
#undef PACKAGE

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_INFO "NRF52840DK-" TERBIUM_BUILD_VARIANT

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * The platform provides an otPlatLog() function.
 */
#ifndef OPENTHREAD_CONFIG_LOG_OUTPUT
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time). If `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE` is set, this defines the most
 * verbose log level possible. See `OPENTHREAD_CONFIG_LOG_LEVEL_INIT` to set the initial log level.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_LEVEL
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_NOTE
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPREND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 0
#endif

/**
 * @def OPENTHREAD_CONFIG_LOG_PLATFORM
 *
 * Define to enable platform region logging.
 *
 */
#ifndef OPENTHREAD_CONFIG_LOG_PLATFORM
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
 *
 * Define to 1 to enable otPlatFlash* APIs to support non-volatile storage.
 *
 * When defined to 1, the platform MUST implement the otPlatFlash* APIs instead of the otPlatSettings* APIs.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 */
#ifndef OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 60
#endif

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#ifndef OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 3
#endif

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#ifndef OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 20
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
 *
 * Define to 1 if you want to enable software ACK timeout logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_ACK_TIMEOUT_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
 *
 * Define to 1 if you want to enable software retransmission logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
 *
 * Define to 1 if you want to enable software CSMA-CA backoff logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Define to 1 if you want to enable software transmission security logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
 *
 * Define to 1 to enable software transmission target time logic.
 *
 */
#ifndef OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE (OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2)
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
 *
 * Define to 1 if you want to use customer defined function 'otPlatRadioGetIeeeEui64()'.
 *
 */
#ifndef OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE
#define OPENTHREAD_CONFIG_ENABLE_PLATFORM_EUI64_CUSTOM_SOURCE 1
#endif

#if defined(TERBIUM_BUILD_ENG)

/**
 * @def OPENTHREAD_CONFIG_DIAG_ENABLE
 *
 * Define to 1 to enable Factory Diagnostics support.
 *
 */
#ifndef OPENTHREAD_CONFIG_DIAG_ENABLE
#define OPENTHREAD_CONFIG_DIAG_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode output buffer size in bytes (Default: 256).
 *
 */
#ifndef OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 500
#endif
#endif // defined(TERBIUM_BUILD_ENG)

//-------------openthread/example/platform/utils configurations---------------//

/**
 * @def SETTINGS_CONFIG_BASE_ADDRESS
 *
 * The base address of settings.
 *
 */
#ifndef SETTINGS_CONFIG_BASE_ADDRESS
#define SETTINGS_CONFIG_BASE_ADDRESS 0
#endif

/**
 * @def SETTINGS_CONFIG_PAGE_SIZE
 *
 * The page size of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_SIZE
#define SETTINGS_CONFIG_PAGE_SIZE 4096
#endif

/**
 * @def SETTINGS_CONFIG_PAGE_NUM
 *
 * The page number of settings.
 *
 */
#ifndef SETTINGS_CONFIG_PAGE_NUM
#define SETTINGS_CONFIG_PAGE_NUM 2
#endif

/**
 * @def LOG_PARSE_BUFFER_SIZE
 *
 * LOG buffer used to parse print format. It will be locally allocated on the
 * stack.
 *
 */
#ifndef LOG_PARSE_BUFFER_SIZE
#define LOG_PARSE_BUFFER_SIZE 250
#endif

/**
 * @def RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM
 *
 * The number of short source address table entries.
 *
 */
#ifndef RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM
#define RADIO_CONFIG_SRC_MATCH_SHORT_ENTRY_NUM 0
#endif

/**
 * @def RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
 *
 * The number of extended source address table entries.
 *
 */
#ifndef RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM
#define RADIO_CONFIG_SRC_MATCH_EXT_ENTRY_NUM 0
#endif

/**
 * @def OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW
 *
 * CSL sample window in units of 10 symbols.
 *
 */
#ifndef OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW
#define OPENTHREAD_CONFIG_CSL_SAMPLE_WINDOW 30
#endif

/*
 * Suppress the ARMCC warning on unreachable statement,
 * e.g. break after assert(false) or ExitNow() macro.
 */
#if defined(__CC_ARM)
_Pragma("diag_suppress=111") _Pragma("diag_suppress=128")
#endif

#endif // OPENTHREAD_CORE_PROJECT_CONFIG_H_
