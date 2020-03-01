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

#ifndef PLATFORM_WIRELESS_CAL_H__
#define PLATFORM_WIRELESS_CAL_H__

#include <terbium-config.h>

#include <stdint.h>

#include <openthread/error.h>
#include <openthread/platform/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_WIRELESSCAL_INVALID_TARGET_POWER INT16_MAX ///< Invalid target power.

/**
 * This enumeration defines the wireless calibration technology type.
 *
 */
enum
{
    kWirelessCalTechnologyTypeUnknown  = 0, ///< Unknown type.
    kWirelessCalTechnologyTypeEthernet = 1, ///< Ethernet.
    kWirelessCalTechnologyTypeWiFi     = 2, ///< Wifi.
    kWirelessCalTechnologyType6LoWPAN  = 3, ///< 6Lowpan.
};

/**
 * This type represents wireless calibration technology type.
 *
 */
typedef uint8_t tbWirelessCalTechnologyType;

// clang-format off

// Temporarily disable GCC option '-Wpedantic' to avoid
// "error:invalid use of structure with flexible array member."
#if defined(__GNUC__)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#endif

/**
 * @struct tbWirelessCalPowerSetting
 *
 * This structure represents wireless calibration power setting.
 *
 */
OT_TOOL_PACKED_BEGIN
struct tbWirelessCalPowerSetting
{
    int16_t mCalibratedPower; ///< Factory measured output power, in 0.01 dBm (e.g. 1575 == 15.75dBm).
    int32_t mEncodedPower;    ///< Encoded power.
} OT_TOOL_PACKED_END;

// clang-format on

/**
 * This structure represents wireless calibration power setting.
 */
typedef struct tbWirelessCalPowerSetting tbWirelessCalPowerSetting;

/**
 * @struct tbWirelessCalSubbandSetting
 *
 * This structure represents wireless calibration subband setting.
 *
 */
OT_TOOL_PACKED_BEGIN
struct tbWirelessCalSubbandSetting
{
    uint32_t                  mFrequencyStart;   ///< Start frequency, in Herz.
    uint32_t                  mFrequencyEnd;     ///< End frequency, in Herz.
    uint8_t                   mNumPowerSettings; ///< Number of power settings.
    tbWirelessCalPowerSetting mPowerSettings[];  ///< Power settings table.
} OT_TOOL_PACKED_END;

/**
 * This structure represents wireless calibration subband setting.
 */
typedef struct tbWirelessCalSubbandSetting tbWirelessCalSubbandSetting;

/**
 * @struct tbWirelessCalSubbandTarget
 *
 * This structure represents wireless calibration subband target.
 *
 */
OT_TOOL_PACKED_BEGIN
struct tbWirelessCalSubbandTarget
{
    uint32_t mFrequencyStart; ///< Start frequency, in Herz.
    uint32_t mFrequencyEnd;   ///< End frequency, in Herz.
    int16_t  mTargetPower;    ///< Target power, in 0.01 dBm.
} OT_TOOL_PACKED_END;

/**
 * This structure represents wireless calibration subband target.
 */
typedef struct tbWirelessCalSubbandTarget tbWirelessCalSubbandTarget;

/**
 * @struct tbWirelessCalTarget
 *
 * This structure represents wireless calibration target.
 *
 */
OT_TOOL_PACKED_BEGIN
struct tbWirelessCalTarget
{
    uint16_t                   mRegulatoryCode;                                         ///< Regulatory code.
    tbWirelessCalSubbandTarget mSubbandTargets[TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND]; ///< Sub-band target table.
} OT_TOOL_PACKED_END;

/**
 * This structure represents wireless calibration target.
 */
typedef struct tbWirelessCalTarget tbWirelessCalTarget;

/**
 * @struct tbWirelessCalParameter
 *
 * This structure represents wireless calibration parameter.
 *
 */
OT_TOOL_PACKED_BEGIN
struct tbWirelessCalParameter
{
    tbWirelessCalTechnologyType mType; ///< Wireless calibration technology type.
    tbWirelessCalTarget
        mTargetPowers[TERBIUM_CONFIG_WIRELESSCAL_NUM_REGULATORY_DOMAIN]; ///< Wireless calibration target table.
    tbWirelessCalSubbandSetting
        mSubbandSettings[TERBIUM_CONFIG_WIRELESSCAL_NUM_SUBBAND]; ///< Wireless calibration subband setting table.
} OT_TOOL_PACKED_END;

/**
 * This structure represents wireless calibration parameter.
 */
typedef struct tbWirelessCalParameter tbWirelessCalParameter;

// clang-format off

// Re-enable GCC option '-Wpedantic'.
#if defined(__GNUC__)
_Pragma("GCC diagnostic pop")
#endif

/**
 * This structure represents wireless calibration.
 */
typedef struct tbWirelessCalCalibration
{
    const char *                  mIdentifier; ///< Unique identifier used for lookup.
    const tbWirelessCalParameter *mParameters; ///< Calibration parameters themselves.
} tbWirelessCalCalibration;

// clang-format on

/**
 * This macro gets the size of subband setting.
 *
 * @param[in] aNumPowerSettings  Number of power settings in the subband setting.
 *
 * @returns The size of subbband setting.
 *
 */
#define WIRELESS_CAL_SUBBAND_SETTING_SIZE(aNumPowerSettings) \
    (sizeof(tbWirelessCalSubbandSetting) + (aNumPowerSettings) * sizeof(tbWirelessCalPowerSetting))

/**
 * This macro gets the next subband setting.
 *
 * @param[in] aSubbandSetting  A pointer to a subband setting.
 *
 * @returns The pointer to next subbband setting.
 *
 */
#define TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT(aSubbandSetting)      \
    (tbWirelessCalSubbandSetting *)((uint8_t *)(aSubbandSetting) + \
                                    WIRELESS_CAL_SUBBAND_SETTING_SIZE((aSubbandSetting)->mNumPowerSettings))

/**
 * This macro gets the next const subband setting.
 *
 * @param[in] aSubbandSetting  A pointer to a subband setting.
 *
 * @returns The const pointer to next subbband setting.
 *
 */
#define TB_WIRELESS_CAL_SUBBAND_SETTING_NEXT_CONST(aSubbandSetting)            \
    (const tbWirelessCalSubbandSetting *)((const uint8_t *)(aSubbandSetting) + \
                                          WIRELESS_CAL_SUBBAND_SETTING_SIZE((aSubbandSetting)->mNumPowerSettings))

/**
 * This function checks whether the channel is valid.
 *
 * @param[in] aChannel  The channel.
 *
 * @returns TRUE if the channel is valid, FALSE otherwise.
 *
 */
bool tbWirelessCalIsChannelValid(uint8_t aChannel);

/**
 * This function checks whether the wireless calibration power setting is valid.
 *
 * @param[in] aPowerSetting  A pointer to wireless calibration power setting.
 *
 * @returns TRUE if the wireless calibration power setting is valid, FALSE otherwise.
 *
 */
bool tbWirelessCalPowerSettingIsValid(const tbWirelessCalPowerSetting *aPowerSetting);

/**
 * This function checks whether the wireless calibration target is valid.
 *
 * @param[in] aTarget  A pointer to wireless calibration target.
 *
 * @returns TRUE if the wireless calibration target is valid, FALSE otherwise.
 *
 */
bool tbWirelessCalTargetIsValid(const tbWirelessCalTarget *aTarget);

/**
 * This function checks whether the wireless calibration subband target is valid.
 *
 * @param[in] aSubbandTarget  A pointer to wireless calibration subband target.
 *
 * @returns TRUE if the wireless calibration subband target is valid, FALSE otherwise.
 *
 */
bool tbWirelessCalSubbandTargetIsValid(const tbWirelessCalSubbandTarget *aSubbandTarget);

/**
 * This function checks whether the wireless calibration subband setting is valid.
 *
 * @param[in] aSubbandSetting  A pointer to wireless calibration subband setting.
 *
 * @returns TRUE if the wireless calibration subband setting is valid, FALSE otherwise.
 *
 */
bool tbWirelessCalSubbandSettingIsValid(const tbWirelessCalSubbandSetting *aSubbandSetting);

/**
 * This function gets the wireless calibration structure.
 *
 * @returns  A pointer to wireless calibration structure.
 *
 */
const tbWirelessCalCalibration *tbWirelessCalGetCalibration(void);

/**
 * This function initializes wireless calibration table.
 *
 */
void tbWirelessCalInit(void);

/**
 * This function maps the channel to frequency.
 *
 * @param[in] aChannel  The channel.
 *
 * @returns  The frequency.
 *
 */
uint32_t tbWirelessCalMapFrequency(uint8_t aChannel);

/**
 * This function maps the frequency to channel.
 *
 * @param[in] aFrequency  The frequency.
 *
 * @returns  The channel.
 *
 */
uint8_t tbWirelessCalMapChannel(uint32_t aFrequency);

/**
 * This function lookup the target power for given regulatory code and frequency.
 *
 * @param[in]  aRegulatoryCode  The regulatory code.
 * @param[in]  aFrequency       The frequency.
 * @param[out] aTargetPower     The target power, in 0.01 dBm.
 *
 * @retval OT_ERROR_NONE       Successfully retrived the target power.
 * @retval OT_ERROR_NOT_FOUND  No target power was found.
 *
 */
otError tbWirelessCalLookupTargetPower(uint16_t aRegulatoryCode, uint32_t aFrequency, int16_t *aTargetPower);

/**
 * This function lookup the wireless calibration power setting for given frequency and target power.
 *
 * @param[in]  aFrequency        The frequency.
 * @param[in]  aTargetPower      The target power, in 0.01 dBm.
 * @param[out] aCalibratedPower  The calibrated power, in 0.01 dBm.
 * @param[out] aEncodedPower     The encoded power.
 *
 * @retval OT_ERROR_NONE       Successfully retrived the power setting.
 * @retval OT_ERROR_FAILED     Failed to get calibration parameter from sysenv.
 * @retval OT_ERROR_NOT_FOUND  No wireless calibration power setting was found.
 *
 */
otError tbWirelessCalLookupPowerSetting(uint32_t aFrequency,
                                        int16_t  aTargetPower,
                                        int16_t *aCalibratedPower,
                                        int32_t *aEncodedPower);

/**
 * This function saves wireless clalibration table to sysenv.
 *
 * @param[in]  aCalibration   A pointe to wireless calibration structure.
 * @param[in]  aBytesToWrite  Number of bytes to write to sysenv.
 *
 * @note This method is used when feature TERBIUM_CONFIG_SYSENV_WRITE_ENABLE is enabled.
 *
 * @retval OT_ERROR_NONE          Successfully retrived the power setting.
 * @retval OT_ERROR_INVALID_ARGS  @p aCalibration or aCalibration->mParameters is NULL or @p aBytesToWrite is 0.
 * @retval OT_ERROR_FAILED        Failed to write calibration parameter to sysenv.
 *
 */
otError tbWirelessCalSave(const tbWirelessCalCalibration *aCalibration, uint32_t aBytesToWrite);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_WIRELESS_CAL_H__
