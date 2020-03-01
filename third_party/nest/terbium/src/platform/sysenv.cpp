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
 *   This file implements an interface for system environment variables.
 *
 */

#include <terbium-config.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>

#include "chips/include/flash.h"
#include "platform/sysenv.h"

#define SYSENV_INVALID_LENGTH 0xffffffff ///< Invalid length value.
#define SYSENV_CRC_SEED 0x00000000       ///< CRC seed.
#define SYSENV_INVALID_CRC 0xffffffff    ///< Invalid CRC value.
#define SYSENV_HEADER_SIZE 4             ///< Header is just a 32-bit CRC.
#define SYSENV_SIZE \
    (TERBIUM_MEMORY_SYSENV_SIZE - SYSENV_HEADER_SIZE) ///< Maximum length of storing system environment variables.

#if TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE > SYSENV_SIZE
#error "TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE larger than SYSENV_SIZE!"
#endif

/*
 * Sysenv memory map:
 *
 * +--------+-------+-------------------------------------------+-----+----------+--------------+
 * |Fields: |Header |                 Entry 0                   | ... |  Entry N |  EmptyField  |
 * +--------+-------+---------+---------+-----------------------+-----+----------+--------------+
 * |Octets: |   4   |    4    | key.len |     4     | value.len | ... |    ...   |              |
 * +--------+-------+---------+---------+-----------+-----------+-----+----------+--------------+
 * |Content:|  crc  | key.len | key.data| value.len | value.data| ... |    ...   |     0xFF     |
 * +--------+-------+---------+---------+-----------+-----------+-----+----------+--------------+
 *
 * - "Header"     : Header is the first 32-bit unsigned integer of the sysenv flash.
 *                  It contains a 32-bit CRC equivalent to ANSI X3.66-1979. The CRC
 *                  is calculated over all Entries and the EmptyField.
 * - "Entry"      : An Entry contains a key-value tuple.  An entry includes four fields:
 *                  -- 'key.len' is the length of the key.
 *                  -- 'key.data' is the content of the key.
 *                  -- 'value.len' is the length of the value.
 *                  -- 'value.data' is the content of the value.
 * - "EmptyField" : EmptyField is the remaining filed of the sysenv flash except
 *                  Header and Entry fields. The EmptyField field is filled with 0xFF.
 */

/**
 * This structure represents the environment variable buffer.
 */
typedef struct EnvBuffer
{
    uint32_t    mLength; ///< Length of the data.
    void const *mData;   ///< The data.
} EnvBuffer;

/**
 * This structure represents the environment variable entry.
 */
typedef struct EnvEntry
{
    EnvBuffer mKey;   ///< Key buffer.
    EnvBuffer mValue; ///< Value buffer.
} EnvEntry;

static uint32_t sCachedCrcValue = 0;

static inline uint32_t crc32_4bit_append(uint32_t aCrc, uint8_t aByte)
{
    /* ANSI X3.66-1979 CRC-32 polynomial:
     * x^0 + x^1 + x^2 + x^4 + x^5 + x^7 + x^8 + x^10 + x^11 + x^12 + x^16 + x^22 + x^23 + x^26 + x^32
     */
    static const uint32_t sCrc32Table[16] = {0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4,
                                             0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
                                             0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c};

    // A byte is processed a 4-bits at a time.
    aCrc = (aCrc >> 4) ^ sCrc32Table[(aCrc ^ aByte) & 0xf];
    aCrc = (aCrc >> 4) ^ sCrc32Table[(aCrc ^ (aByte >> 4)) & 0xf];

    return aCrc;
}

static uint32_t crc32_4bit(uint32_t aCrc, const uint8_t *aBuffer, uint32_t aLength, uint8_t aPad, uint32_t aPadLength)
{
    aCrc = ~aCrc;
    while (aLength--)
    {
        aCrc = crc32_4bit_append(aCrc, *aBuffer++);
    }

    while (aPadLength--)
    {
        aCrc = crc32_4bit_append(aCrc, aPad);
    }

    return ~aCrc;
}

static uint32_t calculateSysenvCrc(const uint8_t *aBuffer, uint32_t aLength)
{
    // The sysenv uses the length SYSENV_SIZE to calculate CRC in Marble. So
    // the RAM buffer size in Marble is set to TERBIUM_MEMORY_SYSENV_SIZE.
    // The size of MEMORY_SYSENV_SIZE is too long, to reduce the RAM buffer
    // size and compatible with Marble, we add pading bytes to calculate CRC
    // here.
    assert(aLength <= TERBIUM_MEMORY_SYSENV_SIZE);

    return crc32_4bit(SYSENV_CRC_SEED, aBuffer + SYSENV_HEADER_SIZE, aLength - SYSENV_HEADER_SIZE, 0xff,
                      TERBIUM_MEMORY_SYSENV_SIZE - aLength);
}

static inline uint16_t sysenvEntrySize(const EnvEntry *aEntry)
{
    return (sizeof(aEntry->mKey.mLength) + aEntry->mKey.mLength + sizeof(aEntry->mValue.mLength) +
            aEntry->mValue.mLength);
}

static int32_t sysenvBufferParseEntry(const uint8_t *aBuffer, EnvEntry *aEntry, uint32_t aBytesLeft)
{
    int32_t        retlen = -1;
    uint32_t       len;
    const uint8_t *data;

    len = *(uint32_t *)aBuffer;

    VerifyOrExit((len != SYSENV_INVALID_LENGTH) && (len < aBytesLeft), OT_NOOP);

    // Get the key.
    data                 = aBuffer + sizeof(len);
    aEntry->mKey.mLength = len;
    aEntry->mKey.mData   = reinterpret_cast<const void *>(data);

    retlen = sizeof(aEntry->mKey.mLength) + aEntry->mKey.mLength;

    // Get the value.
    data += len;
    len = *reinterpret_cast<const uint32_t *>(data);
    data += sizeof(len);
    aEntry->mValue.mLength = len;
    aEntry->mValue.mData   = reinterpret_cast<const void *>(data);

    retlen += sizeof(aEntry->mValue.mLength) + aEntry->mValue.mLength;

exit:
    return retlen;
}

static otError sysenvBufferFindEntry(const EnvBuffer *aBuffer,
                                     const EnvBuffer *aKey,
                                     EnvEntry *       aEntry,
                                     const uint8_t ** aNextLocation)
{
    otError        error     = OT_ERROR_NOT_FOUND;
    uint32_t       bytesLeft = aBuffer->mLength;
    uint32_t       bytesRead;
    int32_t        len;
    const uint8_t *nextLocation;

    nextLocation = reinterpret_cast<const uint8_t *>(aBuffer->mData);

    do
    {
        len = sysenvBufferParseEntry(nextLocation, aEntry, bytesLeft);

        if (len > 0)
        {
            // Move on to the next entry.
            nextLocation += len;

            // If looking for a match.
            if ((aKey != NULL) && (aKey->mLength == aEntry->mKey.mLength) &&
                !memcmp(aKey->mData, aEntry->mKey.mData, aEntry->mKey.mLength))
            {
                error = OT_ERROR_NONE;
                break;
            }
        }

        bytesRead = (nextLocation - reinterpret_cast<const uint8_t *>(aBuffer->mData));
        bytesLeft = aBuffer->mLength - bytesRead;
    } while ((len > 0) && (bytesRead < aBuffer->mLength));

    if (aNextLocation != NULL)
    {
        *aNextLocation = nextLocation;
    }

    return error;
}

static otError sysenvReadAndCheck(EnvBuffer *aBuffer)
{
    otError        error      = OT_ERROR_NONE;
    const uint8_t *flashStart = reinterpret_cast<const uint8_t *>(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS);
    uint32_t       crcRead    = *(reinterpret_cast<const uint32_t *>(flashStart));

    // Point to the internal flash memory, skipping past the header to the data.
    aBuffer->mData   = reinterpret_cast<const void *>(flashStart + SYSENV_HEADER_SIZE);
    aBuffer->mLength = TERBIUM_MEMORY_SYSENV_SIZE - SYSENV_HEADER_SIZE;

    // Check if flash is empty.
    if (crcRead == SYSENV_INVALID_CRC)
    {
        otLogNotePlat("%s: Flash is empty", __func__);
        ExitNow();
    }

    // If the flash is not empty, check the CRC.
    if (sCachedCrcValue == 0 || sCachedCrcValue != crcRead)
    {
        uint32_t crcCalc = calculateSysenvCrc(flashStart, TERBIUM_MEMORY_SYSENV_SIZE);

        if (crcRead != crcCalc)
        {
            error = OT_ERROR_FAILED;
            otLogWarnPlat("%s: Check CRC failed, crcRead=%08x, crcCalc=%08x", __func__, crcRead, crcCalc);
        }
        else
        {
            sCachedCrcValue = crcCalc;
        }
    }

exit:
    return error;
}

static otError sysenvFindEntry(const char *aKey, EnvEntry *aEntry)
{
    otError   error;
    EnvBuffer buffer;
    EnvBuffer key;

    VerifyOrExit((aKey != NULL) && (aEntry != NULL), error = OT_ERROR_INVALID_ARGS);

    key.mData   = reinterpret_cast<const void *>(aKey);
    key.mLength = strlen(aKey);

    // Read data from sysenv flash to EnvBuffer and check the CRC.
    VerifyOrExit((error = sysenvReadAndCheck(&buffer)) == OT_ERROR_NONE, OT_NOOP);

    // Find the entry from the EnvBuffer.
    VerifyOrExit((error = sysenvBufferFindEntry(&buffer, &key, aEntry, NULL)) == OT_ERROR_NONE, OT_NOOP);

exit:
    return error;
}

otError sysenvGetNext(uint32_t *   aIterator,
                      const char **aKey,
                      uint32_t *   aKeyLen,
                      const void **aValue,
                      uint32_t *   aValueLen)
{
    otError               error = OT_ERROR_NONE;
    EnvEntry              entry;
    const uint8_t *       bufferStart;
    static const uint8_t *sBufferEnd;

    VerifyOrExit(aIterator != NULL, error = OT_ERROR_INVALID_ARGS);

    if (*aIterator == TB_SYSENV_ITERATOR_INIT)
    {
        EnvBuffer buffer;

        VerifyOrExit((error = sysenvReadAndCheck(&buffer)) == OT_ERROR_NONE, OT_NOOP);
        bufferStart = reinterpret_cast<const uint8_t *>(buffer.mData);
        sBufferEnd  = bufferStart + buffer.mLength;
    }
    else
    {
        bufferStart = (const uint8_t *)(*aIterator);
    }

    // Check whether the address exceeds the flash range.
    VerifyOrExit((bufferStart >= reinterpret_cast<const uint8_t *>(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS)) &&
                     (bufferStart < reinterpret_cast<const uint8_t *>(TERBIUM_MEMORY_SYSENV_TOP_ADDRESS)),
                 error = OT_ERROR_NOT_FOUND);

    // Parse entry from the given address.
    VerifyOrExit(sysenvBufferParseEntry(bufferStart, &entry, sBufferEnd - bufferStart) > 0, error = OT_ERROR_NOT_FOUND);

    // Move on to the next entry.
    *aIterator = reinterpret_cast<uint32_t>(bufferStart + sysenvEntrySize(&entry));

    if (aKey != NULL)
    {
        *aKey = reinterpret_cast<const char *>(entry.mKey.mData);
    }

    if (aKeyLen != NULL)
    {
        *aKeyLen = entry.mKey.mLength;
    }

    if (aValue != NULL)
    {
        *aValue = entry.mValue.mData;
    }

    if (aValueLen != NULL)
    {
        *aValueLen = entry.mValue.mLength;
    }

exit:
    return error;
}

otError tbSysenvGetNext(uint32_t *   aIterator,
                        const char **aKey,
                        uint32_t *   aKeyLen,
                        const void **aValue,
                        uint32_t *   aValueLen)
{
    return sysenvGetNext(aIterator, aKey, aKeyLen, aValue, aValueLen);
}

otError tbSysenvGetStats(tbSysenvStats *aStats)
{
    otError  error = OT_ERROR_NONE;
    EnvEntry entry;
    uint32_t iterator     = TB_SYSENV_ITERATOR_INIT;
    uint8_t  metadataSize = sizeof(entry.mKey.mLength) + sizeof(entry.mValue.mLength);

    VerifyOrExit(aStats != NULL, error = OT_ERROR_INVALID_ARGS);

    memset(aStats, 0, sizeof(*aStats));
    aStats->mHeaderBytes = SYSENV_HEADER_SIZE;
    aStats->mTotalBytes  = SYSENV_HEADER_SIZE;
    aStats->mMaxBytes    = SYSENV_SIZE;

    while (sysenvGetNext(&iterator, NULL, &entry.mKey.mLength, NULL, &entry.mValue.mLength) == OT_ERROR_NONE)
    {
        aStats->mNumEntries++;
        aStats->mKeyBytes += entry.mKey.mLength;
        aStats->mValueBytes += entry.mValue.mLength;
        aStats->mMetadataBytes += metadataSize;
        aStats->mTotalBytes += metadataSize + entry.mKey.mLength + entry.mValue.mLength;
    }

exit:
    return error;
}

otError tbSysenvGetPointer(const char *aKey, const void **aValue, uint32_t *aValueLen)
{
    otError  error;
    EnvEntry entry;

    VerifyOrExit((aKey != NULL) && (aValue != NULL) && (aValueLen != NULL), error = OT_ERROR_INVALID_ARGS);

    // Find the entry for the given key from sysenv flash.
    VerifyOrExit((error = sysenvFindEntry(aKey, &entry)) == OT_ERROR_NONE, OT_NOOP);

    // Directly return the address of the value data in sysenv flash.
    *aValue    = entry.mValue.mData;
    *aValueLen = entry.mValue.mLength;

    otLogDebgPlat("%s: key=%s len=%d", __func__, aKey, *aValueLen);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogNotePlat("%s: error=%d, key \"%s\" was not found.", __func__, error, aKey);
    }
    return error;
}

otError tbSysenvGet(const char *aKey, void *aValue, uint32_t *aValueLen)
{
    otError  error;
    EnvEntry entry;

    VerifyOrExit((aKey != NULL) && (aValue != NULL) && (aValueLen != NULL), error = OT_ERROR_INVALID_ARGS);

    // Find the entry for the given key from sysenv flash.
    VerifyOrExit((error = sysenvFindEntry(aKey, &entry)) == OT_ERROR_NONE, OT_NOOP);
    VerifyOrExit(entry.mValue.mLength <= *aValueLen, error = OT_ERROR_NO_BUFS);

    // Copy the value data from flash to user buffer.
    memcpy(aValue, entry.mValue.mData, entry.mValue.mLength);
    *aValueLen = entry.mValue.mLength;

    otLogDebgPlat("%s: key=%s len=%d", __func__, aKey, *aValueLen);

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogNotePlat("%s: error=%d, key \"%s\" was not found.", __func__, error, aKey);
    }

    return error;
}

#if TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
static tbSysenvNotifier *sNotifierlist = NULL;

static otError sysenvBufferSetEntry(EnvBuffer *aBuffer, const EnvEntry *aEntry)
{
    otError  error;
    uint8_t *nextLocation;
    EnvEntry foundEntry;

    // First check if the value is already there, remove the entry.
    error = sysenvBufferFindEntry(aBuffer, &aEntry->mKey, &foundEntry, const_cast<const uint8_t **>(&nextLocation));
    if (error == OT_ERROR_NONE)
    {
        // The start address of the found entry.
        uint8_t *foundEntryStart = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(foundEntry.mKey.mData)) -
                                   sizeof(foundEntry.mKey.mLength);
        // The end address of the found entry.
        uint8_t *foundEntryEnd = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(foundEntry.mValue.mData)) +
                                 foundEntry.mValue.mLength;
        // Number of bytes to move.
        uint32_t moveNumBytes = aBuffer->mLength - (foundEntryEnd - reinterpret_cast<const uint8_t *>(aBuffer->mData));
        // Where to start clearing from.
        uint8_t *trailLocation = foundEntryStart + moveNumBytes;

        // Need to move everything at nextLocation and after to foundEntry.mKey.mData.
        memmove(foundEntryStart, nextLocation, moveNumBytes);

        // Then clear the trailing bytes at the end.
        memset(trailLocation, 0xff, sysenvEntrySize(&foundEntry));
    }

    // Now, if setting it to something, add the entry to the end of the buffer.
    if (aEntry->mValue.mData != NULL)
    {
        // Find the next empty slot (foundEntry is just dummy at this point).
        sysenvBufferFindEntry(aBuffer, NULL, &foundEntry, const_cast<const uint8_t **>(&nextLocation));

        // Check that there's enough space for the new entry.
        if (static_cast<uint32_t>((nextLocation + sysenvEntrySize(aEntry)) -
                                  reinterpret_cast<const uint8_t *>(aBuffer->mData)) <= aBuffer->mLength)
        {
            // Copy the entry to this location.
            memcpy(nextLocation, &aEntry->mKey.mLength, sizeof(aEntry->mKey.mLength));
            nextLocation += sizeof(aEntry->mKey.mLength);
            memcpy(nextLocation, aEntry->mKey.mData, aEntry->mKey.mLength);
            nextLocation += aEntry->mKey.mLength;
            memcpy(nextLocation, &aEntry->mValue.mLength, sizeof(aEntry->mValue.mLength));
            nextLocation += sizeof(aEntry->mValue.mLength);
            memcpy(nextLocation, aEntry->mValue.mData, aEntry->mValue.mLength);

            error = OT_ERROR_NONE;
        }
        else
        {
            otLogWarnPlat("%s: No enough buffer.", __func__);
            error = OT_ERROR_NO_BUFS;
        }
    }

    return error;
}

static otError sysenvReadFlash(uint8_t *aBuffer, uint32_t aBufferSize)
{
    otError  error = OT_ERROR_NONE;
    uint32_t crcRead;
    uint32_t bytesRead;

    assert(aBufferSize <= SYSENV_SIZE);

    // Read the whole env region of flash into the temp RAM buffer.
    bytesRead = tbHalFlashRead(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS, aBuffer, aBufferSize);
    if (bytesRead != aBufferSize)
    {
        otLogWarnPlat("%s: Read flash failed: address=%08x, size=%08x", __func__, TERBIUM_MEMORY_SYSENV_BASE_ADDRESS,
                      aBufferSize);
        ExitNow(error = OT_ERROR_FAILED);
    }

    // Get the CRC that was read in.
    crcRead = *reinterpret_cast<uint32_t *>(aBuffer);

    if (crcRead != SYSENV_INVALID_CRC)
    {
        // If the region is not empty, check the CRC.
        if (crcRead != calculateSysenvCrc(aBuffer, aBufferSize))
        {
            otLogWarnPlat("%s: Check CRC failed. Please check if the "
                          "TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE is set too small.",
                          __func__);
            ExitNow(error = OT_ERROR_FAILED);
        }
    }
    else
    {
        otLogNotePlat("%s: Flash is empty", __func__);
    }

exit:
    return error;
}

static otError sysenvWriteFlash(uint8_t *aBuffer, uint32_t aBufferSize)
{
    otError   error;
    uint32_t  bytesWrite;
    uint32_t *crcPtr;

    assert(aBufferSize <= SYSENV_SIZE);

    // Point the pointer to the CRC filed.
    crcPtr = reinterpret_cast<uint32_t *>(aBuffer);

    // Calculate CRC on new data aBuffer, replace the previous CRC with the new one.
    *crcPtr = calculateSysenvCrc(aBuffer, aBufferSize);

    // Erase the entire sysenv area.
    error = tbHalFlashErasePage(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS, TERBIUM_MEMORY_SYSENV_SIZE);
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: Erase flash page failed: address=%08x, size=%08x", __func__,
                      TERBIUM_MEMORY_SYSENV_BASE_ADDRESS, TERBIUM_MEMORY_SYSENV_SIZE);
        ExitNow();
    }

    // Write the entire RAM buffer to flash.
    bytesWrite = tbHalFlashWrite(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS, aBuffer, aBufferSize);
    if (bytesWrite != aBufferSize)
    {
        otLogWarnPlat("%s: Write flash failed: address=%08x, size=%08x", __func__, TERBIUM_MEMORY_SYSENV_BASE_ADDRESS,
                      aBufferSize);
        ExitNow(error = OT_ERROR_FAILED);
    }

    sCachedCrcValue = 0;

exit:
    return error;
}

static otError sysenvWriteEntryToFlash(const EnvEntry *aEntry)
{
    uint8_t   ramBuffer[TERBIUM_CONFIG_SYSENV_RAM_BUFFER_SIZE];
    otError   error;
    EnvBuffer buffer;

    // Read data from flash to RAM buffer.
    VerifyOrExit((error = sysenvReadFlash(ramBuffer, sizeof(ramBuffer))) == OT_ERROR_NONE, OT_NOOP);

    // Skip past the header to the data.
    buffer.mData   = ramBuffer + SYSENV_HEADER_SIZE;
    buffer.mLength = sizeof(ramBuffer) - SYSENV_HEADER_SIZE;

    // Write the entry to RAM buffer.
    VerifyOrExit((error = sysenvBufferSetEntry(&buffer, aEntry)) == OT_ERROR_NONE, OT_NOOP);

    // Write the RAM buffer to flash.
    VerifyOrExit((error = sysenvWriteFlash(ramBuffer, sizeof(ramBuffer))) == OT_ERROR_NONE, OT_NOOP);

exit:
    return error;
}

otError tbSysenvSet(const char *aKey, const void *aValue, uint32_t aValueLen)
{
    otError           error;
    EnvEntry          entry;
    tbSysenvNotifier *notifier = sNotifierlist;

    VerifyOrExit(aKey != NULL, error = OT_ERROR_INVALID_ARGS);

    // Build an entry.
    entry.mKey.mData     = reinterpret_cast<const void *>(aKey);
    entry.mKey.mLength   = strlen(aKey);
    entry.mValue.mData   = aValue;
    entry.mValue.mLength = aValueLen;

    VerifyOrExit((error = sysenvWriteEntryToFlash(&entry)) == OT_ERROR_NONE, OT_NOOP);
    otLogDebgPlat("%s: key=%s len=%d", __func__, aKey, aValueLen);

    // The address of key-value entries may have already changed in sysenv,
    // notify users to call 'tbSysenvGetPointer' to reload the pointer from
    // sysenv.
    while (notifier != NULL)
    {
        notifier->mCallback(notifier->mContext);
        notifier = notifier->mNext;
    }

exit:
    if (error != OT_ERROR_NONE)
    {
        otLogWarnPlat("%s: error=%d, write key \"%s\" to sysenv failed.", __func__, error, aKey);
    }

    return error;
}

otError tbSysenvNotifierRegister(tbSysenvNotifier *aNotifier)
{
    otError error = OT_ERROR_NONE;

    VerifyOrExit((aNotifier != NULL) && (aNotifier->mCallback != NULL), error = OT_ERROR_INVALID_ARGS);

    aNotifier->mNext = sNotifierlist;
    sNotifierlist    = aNotifier;

exit:
    return error;
}
#endif // TERBIUM_CONFIG_SYSENV_WRITE_ENABLE
