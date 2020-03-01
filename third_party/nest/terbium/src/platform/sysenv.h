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
 *   This file includes definitions for Sysenv.
 *
 */

#ifndef PLATFORM_SYSENV_H__
#define PLATFORM_SYSENV_H__

#include <stdint.h>
#include <openthread/error.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_SYSENV_ITERATOR_INIT 0 ///< Initializer for sysenv iterator.

/**
 * This structure represents the statistics information of sysenv.
 *
 */
typedef struct
{
    uint16_t mTotalBytes;    ///< Total number of occupied sysenv bytes.
    uint16_t mMaxBytes;      ///< Maximum number of sysenv bytes.
    uint16_t mHeaderBytes;   ///< Number of sysenv header bytes.
    uint16_t mNumEntries;    ///< Number of sysenv entries.
    uint16_t mKeyBytes;      ///< Number of key bytes.
    uint16_t mValueBytes;    ///< Number of value bytes.
    uint16_t mMetadataBytes; ///< Number of metedata bytes.
} tbSysenvStats;

/**
 * This method pointer is called when a key-value is writen to sysenv.
 *
 * @param[in]  aContext   A pointer to application-specific context.
 *
 */
typedef void (*tbSysenvNotifierCallback)(void *aContext);

/**
 * This structure represents the statistics information of sysenv.
 *
 */
typedef struct tbSysenvNotifier
{
    tbSysenvNotifierCallback mCallback; ///< A pointer to a method that is called when
                                        ///< a key-value tuple is writen to sysenv.
    void *                   mContext;  ///< A pointer to application-specific context.
    struct tbSysenvNotifier *mNext;     ///< A pointer to next notifier entry (internal use only).
} tbSysenvNotifier;

/**
 * This method gets the key-value tuple at aIterator position.
 *
 * @param[inout]  aIterator  An iterator to the position of the key-value tuple.
 *                           To get the first key-value tuple, it should be set
 *                           to TB_SYSENV_ITERATOR_INIT.
 * @param[out]    aKey       A pointer to key string.
 * @param[out]    aKeyLen    A pointer to the length of the key string.
 * @param[out]    aValue     A pointer to value buffer.
 * @param[out]    aValueLen  A pointer to the length of the value buffer.
 *
 * @retval OT_ERROR_NONE          Successfully got the key-value tuple.
 * @retval OT_ERROR_INVALID_ARGS  The @p aIterator is NULL.
 * @retval OT_ERROR_NOT_FOUND     Not found the key-value tuple.
 *
 */
otError tbSysenvGetNext(uint32_t *   aIterator,
                        const char **aKey,
                        uint32_t *   aKeyLen,
                        const void **aValue,
                        uint32_t *   aValueLen);

/**
 * This method registers a sysenv notifier entry to internal notifier list.
 * When a key-value tuple is written to sysenv, sysenv may move other key-value
 * tuples to a new place. So the previous stored position of the key-value
 * tuple may be changed. To resolve this cace, sysenv calls the callback
 * methods in notifier list to notify users to call 'tbSysenvGetPointer()'
 * to reload the pointer.
 *
 * @note This method is used when feature TERBIUM_CONFIG_SYSENV_WRITE_ENABLE is enabled.
 *
 * @param[in]  aNotifier  A pointer to notifier entry.
 *
 * @retval OT_ERROR_NONE          Successfully registered the notifier entry.
 * @retval OT_ERROR_INVALID_ARGS  The @p aNotifier or @p aNotifier->mCallback is NULL.
 *
 */
otError tbSysenvNotifierRegister(tbSysenvNotifier *aNotifier);

/**
 * This method stores the key-value tuple to sysenv.
 *
 * @note This method is used when feature TERBIUM_CONFIG_SYSENV_WRITE_ENABLE is enabled.
 *
 * @param[in]  aKey       A pointer to key string.
 * @param[in]  aValue     A pointer to value buffer.
 * @param[in]  aValueLen  The length of the value buffer.
 *
 * @retval OT_ERROR_NONE          Successfully stored the key and value to sysenv.
 * @retval OT_ERROR_FAILED        Write or read flash failed.
 * @retval OT_ERROR_INVALID_ARGS  The @p aKey is NULL.
 *
 */
otError tbSysenvSet(const char *aKey, const void *aValue, uint32_t aValueLen);

/**
 * This method reads the value from sysenv to buffer.
 *
 * @param[in]     aKey       A pointer to key string.
 * @param[out]    aValue     A pointer to value buffer.
 * @param[inout]  aValueLen  The length of the value buffer.
 *
 * @retval OT_ERROR_NONE          Successfully stored the key and value to sysenv.
 * @retval OT_ERROR_FAILED        Write or read flash failed.
 * @retval OT_ERROR_INVALID_ARGS  The @p aKey or aValue or aValueLen is NULL.
 * @retval OT_ERROR_NO_BUFS       The @p aValue is not large enough to store the value.
 *
 */
otError tbSysenvGet(const char *aKey, void *aValue, uint32_t *aValueLen);

/**
 * This method points the pointer @p aValue to the sysenv internal flash area of the value data。
 *
 * @param[in]  aKey       A pointer to key string.
 * @param[out] aValue     A pointer to the sysenv internal flash area of the value.
 * @param[out] aValueLen  The length of the value buffer.
 *
 * @retval OT_ERROR_NONE          Successfully got the pointer.
 * @retval OT_ERROR_FAILED        Read flash failed.
 * @retval OT_ERROR_NOT_FOUND     There is no corresponding key stored in sysenv.
 * @retval OT_ERROR_INVALID_ARGS  The @p aKey or aValue or aLength is NULL.
 */
otError tbSysenvGetPointer(const char *aKey, const void **aValue, uint32_t *aValueLen);

/**
 * This method gets the sysenv statistic information.
 *
 * @param[out]  aStats   A pointer to sysenv statistic information.
 *
 * @retval OT_ERROR_NONE          Successfully iterated sysenv entries.
 * @retval OT_ERROR_FAILED        Read flash failed.
 * @retval OT_ERROR_INVALID_ARGS  The @p aStats is NULL.
 */
otError tbSysenvGetStats(tbSysenvStats *aStats);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_SYSENV_H__
