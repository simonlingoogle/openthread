/**
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    This document is the property of Nest. It is considered
 *    confidential and proprietary information.
 *
 *    This document may not be reproduced or transmitted in any form,
 *    in whole or in part, without the express written permission of
 *    Nest.
 *
 *    Description:
 *      This file defines a project configuration for OpenThread used by CastOS products.
 *
 */

#ifndef OPENTHREAD_CORE_CASTOS_CONFIG_H_
#define OPENTHREAD_CORE_CASTOS_CONFIG_H_

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_INFO "POSIX-CASTOS"

/**
 * @def OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
 *
 * Define as 1 to enable PTY device support in POSIX app.
 *
 */
#ifndef OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE
#define OPENTHREAD_POSIX_CONFIG_RCP_PTY_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS
 *
 * The number of message buffers in the buffer pool.
 *
 * NOTE: Router devices that support 32 children will have 260 message buffers minimum.
 *       This allows for one 1KB frame per child with a little overhead.
 */
#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 280

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT
 *
 * The maximum number of retries allowed after a transmission failure for direct transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 3.
 *
 */
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 15

/**
 * @def OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT
 *
 * The maximum number of retries allowed after a transmission failure for indirect transmissions.
 *
 * Equivalent to macMaxFrameRetries, default value is 0.
 *
 */
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 1

/**
 * @def OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS
 *
 * Maximum number of transmit attempts for an outbound indirect frame (for a sleepy child) each triggered by the
 * reception of a new data request command (a new data poll) from the sleepy child. Each data poll triggered attempt is
 * retried by the MAC layer up to `OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT` times.
 *
 */
#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 16

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES
 *
 * The number of EID-to-RLOC cache entries.
 *
 */
#define OPENTHREAD_CONFIG_TMF_ADDRESS_CACHE_ENTRIES 32

/**
 * @def OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY
 *
 * Maximum retry delay for address query (in seconds).
 *
 * Replace with 2 min instead of the OT default of 8 hours.
 *
 */
#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY 120

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 32

/**
 * @def OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD
 *
 * The minimum number of supported IPv6 address registrations per child.
 *
 */
#define OPENTHREAD_CONFIG_MLE_IP_ADDRS_PER_CHILD 6

/**
 * @def OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT
 *
 * Define to 1 to send an MLE Link Request when MAX_NEIGHBOR_AGE is reached for a neighboring router.
 *
 * This is enabled to increase reliability of exchanges between routers (default on OpenThread is disabled).
 *
 */
#define OPENTHREAD_CONFIG_MLE_SEND_LINK_REQUEST_ON_ADV_TIMEOUT 1

/**
 * @def OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS
 *
 * The maximum number of state-changed callback handlers (set using `otSetStateChangedCallback()`).
 *
 */
#define OPENTHREAD_CONFIG_MAX_STATECHANGE_HANDLERS 3

/**
 * @def OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE
 *
 * Enable setting steering data out of band.
 *
 * This is used for "joinable" mode during weave pairing
 *
 */
#define OPENTHREAD_CONFIG_MLE_STEERING_DATA_SET_OOB_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION
 *
 * The Beacon version to use when the beacon join flag is set.
 *
 * This is required to enable the to be able to assist older legacy only device (e.g., T2/T1 not running Fuji) during
 * weave pairing flow.
 *
 */
#define OPENTHREAD_CONFIG_MAC_JOIN_BEACON_VERSION 1

/**
 * @def OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE
 *
 * The size of NCP message buffer in bytes
 *
 */
#define OPENTHREAD_CONFIG_NCP_TX_BUFFER_SIZE 3500

/**
 * @def OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode output buffer size.
 *
 */
#define OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE 1290

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX
 *
 * Define OpenThread diagnostic mode max command line arguments.
 *
 */
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_ARGS_MAX 64

/**
 * @def OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE
 *
 * Define OpenThread diagnostic mode command line buffer size
 *
 */
#define OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE 1300

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 3500

/**
 * @def OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
 *
 * The size of CLI UART RX buffer in bytes.
 *
 */
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 3500

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL
 *
 * The log level (used at compile time). If `OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE`
 * is set, this defines the most verbose log level possible.
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_INFO

/**
 * @def OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE
 *
 * Define as 1 to enable dynamic log level control.
 *
 * Note that the OPENTHREAD_CONFIG_LOG_LEVEL determines the log level at
 * compile time. The dynamic log level control (if enabled) only allows
 * decreasing the log level from the compile time value.
 *
 */
#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_LOG_OUTPUT
 *
 * Selects if, and where the LOG output goes to.
 *
 * OPENTHREAD_CONFIG_LOG_OUTPUT_NONE: Log output goes to the bit bucket (disabled)
 * OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED: Log output is handled by a platform defined function
 * OPENTHREAD_CONFIG_LOG_OUTPUT_NCP_SPINEL: Log output for NCP goes to Spinel `STREAM_LOG` property.
 *
 * Log output is set to `PLATFORM_DEFINED` which pushes the log through syslog().
 *
 */
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL
 *
 * Define to prepend the log level to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_LEVEL 1

/**
 * @def OPENTHREAD_CONFIG_LOG_PREPEND_REGION
 *
 * Define to prepend the log region to all log messages
 *
 */
#define OPENTHREAD_CONFIG_LOG_PREPEND_REGION 1

/**
 * @def OPENTHREAD_CONFIG_LOG_SUFFIX
 *
 * Define suffix to append at the end of logs.
 *
 */
#define OPENTHREAD_CONFIG_LOG_SUFFIX ""

/**
 * Enable logs for all regions.
 *
 * Note that particularly, we enable `OPENTHREAD_CONFIG_LOG_PLATFORM` (which is not enabled by default in OT). The
 * platform region is used by radio-spinel platform code interacting with RCP.
 *
 */
#define OPENTHREAD_CONFIG_LOG_API 1
#define OPENTHREAD_CONFIG_LOG_MLE 1
#define OPENTHREAD_CONFIG_LOG_ARP 1
#define OPENTHREAD_CONFIG_LOG_NETDATA 1
#define OPENTHREAD_CONFIG_LOG_ICMP 1
#define OPENTHREAD_CONFIG_LOG_IP6 1
#define OPENTHREAD_CONFIG_LOG_MAC 1
#define OPENTHREAD_CONFIG_LOG_MEM 1
#define OPENTHREAD_CONFIG_LOG_PKT_DUMP 1
#define OPENTHREAD_CONFIG_LOG_NETDIAG 1
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1
#define OPENTHREAD_CONFIG_LOG_CLI 1
#define OPENTHREAD_CONFIG_LOG_COAP 1
#define OPENTHREAD_CONFIG_LOG_CORE 1
#define OPENTHREAD_CONFIG_LOG_UTIL 1

#define OPENTHREAD_CONFIG_LEGACY_ENABLE 1
#define OPENTHREAD_ENABLE_VENDOR_EXTENSION 1

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE
 *
 * Define to 1 if you want to enable radio coexistence implemented in platform.
 *
 */
#define OPENTHREAD_CONFIG_PLATFORM_RADIO_COEX_ENABLE 1

#endif // OPENTHREAD_CORE_CASTOS_CONFIG_H_
