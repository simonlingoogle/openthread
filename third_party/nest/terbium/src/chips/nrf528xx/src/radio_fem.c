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
 *   This file implements the FEM controller functions.
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>

#include <stdint.h>
#include <string.h>

#include "platform-nrf5.h"

#include "chips/include/radio.h"

#ifdef TERBIUM_CONFIG_FEM_PIN_CONTROLLER_ENABLE

#define ENABLE_FEM 1
#include <nrf_802154.h>

#include <common/logging.hpp>

// Configuration parameters for the Front End Module timings and gain.
#define NRF_FEM_CONTROL_PA_TIME_IN_ADVANCE_US \
    13 ///< Time in microseconds when PA GPIO is activated before the radio is ready for transmission.
#define NRF_FEM_CONTROL_LNA_TIME_IN_ADVANCE_US \
    13 ///< Time in microseconds when LNA GPIO is activated before the radio is ready for reception.

#define NRF_FEM_CONTROL_PDN_SETTLE_US 18 ///< The time between activating the PDN and asserting the RX_EN/TX_EN.
#define NRF_FEM_CONTROL_TRX_HOLD_US 5    ///< The time between deasserting the RX_EN/TX_EN and deactivating PDN.
#define NRF_FEM_CONTROL_PA_GAIN_DB 0     ///< PA gain. Ignored if the amplifier is not supporting this feature.
#define NRF_FEM_CONTROL_LNA_GAIN_DB 0    ///< LNA gain. Ignored if the amplifier is not supporting this feature.

// FEM pin PPI channel configuration.
#define NRF_FEM_CONTROL_PDN_PPI_CHANNEL 5          ///< PPI channel for Power Down control.
#define NRF_FEM_CONTROL_SET_PPI_CHANNEL 15         ///< PPI channel for pin setting.
#define NRF_FEM_CONTROL_CLR_PPI_CHANNEL 16         ///< PPI channel for pin clearing.
#define NRF_FEM_CONTROL_CPS_CHL_SET_PPI_CHANNEL 17 ///< PPI channel for setting CPS/CHL pin.
#define NRF_FEM_CONTROL_CPS_CHL_CLR_PPI_CHANNEL 18 ///< PPI channel for clearing CPS/CHL pin.

// FEM pin GPIOTE channel configuration.
#define NRF_FEM_CONTROL_PDN_GPIOTE_CHANNEL 5 ///< PDN GPIOTE channel for FEM control.
#define NRF_FEM_CONTROL_PA_GPIOTE_CHANNEL 6  ///< PA GPIOTE channel for FEM control.
#define NRF_FEM_CONTROL_LNA_GPIOTE_CHANNEL 7 ///< LNA GPIOTE channel for FEM control.
#define NRF_FEM_CONTROL_CPS_GPIOTE_CHANNEL 3 ///< CPS GPIOTE channel for FEM control.
#define NRF_FEM_CONTROL_CHL_GPIOTE_CHANNEL 4 ///< CHL GPIOTE channel for FEM control.

void tbHalRadioFemPinControllerInit(tbHalRadioFemPinControllerConfig *aConfig)
{
    // clang-format off
    nrf_fem_interface_config_t femInterfaceConfig =
    {
        .fem_config =
        {
            .pa_time_gap_us  = NRF_FEM_CONTROL_PA_TIME_IN_ADVANCE_US,
            .lna_time_gap_us = NRF_FEM_CONTROL_LNA_TIME_IN_ADVANCE_US,
            .pdn_settle_us   = NRF_FEM_CONTROL_PDN_SETTLE_US,
            .trx_hold_us     = NRF_FEM_CONTROL_TRX_HOLD_US,
            .pa_gain_db      = NRF_FEM_CONTROL_PA_GAIN_DB,
            .lna_gain_db     = NRF_FEM_CONTROL_LNA_GAIN_DB
        },
        .pa_pin_config =
        {
            .enable       = aConfig->mCtxPinConfig.mEnable,
            .active_high  = aConfig->mCtxPinConfig.mActiveHigh,
            .gpio_pin     = aConfig->mCtxPinConfig.mGpioPin,
            .gpiote_ch_id = NRF_FEM_CONTROL_PA_GPIOTE_CHANNEL
        },
        .lna_pin_config =
        {
            .enable       = aConfig->mCrxPinConfig.mEnable,
            .active_high  = aConfig->mCrxPinConfig.mActiveHigh,
            .gpio_pin     = aConfig->mCrxPinConfig.mGpioPin,
            .gpiote_ch_id = NRF_FEM_CONTROL_LNA_GPIOTE_CHANNEL
        },
        .pdn_pin_config =
        {
            .enable       = aConfig->mCsdPinConfig.mEnable,
            .active_high  = aConfig->mCsdPinConfig.mActiveHigh,
            .gpio_pin     = aConfig->mCsdPinConfig.mGpioPin,
            .gpiote_ch_id = NRF_FEM_CONTROL_PDN_GPIOTE_CHANNEL
        },
        .cps_pin_config =
        {
            .enable       = aConfig->mCpsPinConfig.mEnable,
            .active_high  = aConfig->mCpsPinConfig.mActiveHigh,
            .gpio_pin     = aConfig->mCpsPinConfig.mGpioPin,
            .gpiote_ch_id = NRF_FEM_CONTROL_CPS_GPIOTE_CHANNEL
        },
        .chl_pin_config =
        {
            .enable       = aConfig->mChlPinConfig.mEnable,
            .active_high  = aConfig->mChlPinConfig.mActiveHigh,
            .gpio_pin     = aConfig->mChlPinConfig.mGpioPin,
            .gpiote_ch_id = NRF_FEM_CONTROL_CHL_GPIOTE_CHANNEL
        },
        .cps_chl_ppi_ch_id_set = NRF_FEM_CONTROL_CPS_CHL_SET_PPI_CHANNEL,
        .cps_chl_ppi_ch_id_clr = NRF_FEM_CONTROL_CPS_CHL_CLR_PPI_CHANNEL,

        .ppi_ch_id_set = NRF_FEM_CONTROL_SET_PPI_CHANNEL,
        .ppi_ch_id_clr = NRF_FEM_CONTROL_CLR_PPI_CHANNEL,
        .ppi_ch_id_pdn = NRF_FEM_CONTROL_PDN_PPI_CHANNEL
    };
    // clang-format on

    nrf_fem_interface_configuration_set(&femInterfaceConfig);
}

otError tbHalRadioFemPinControllerActivateModePin(tbHalRadioFemModePin aModePin, bool aActivate)
{
    int32_t retval = nrf_fem_activate_mode_pin((nrf_fem_mode_pin_t)aModePin, aActivate);
    return (retval == NRF_SUCCESS) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}
#endif // TERBIUM_CONFIG_FEM_PIN_CONTROLLER_ENABLE
