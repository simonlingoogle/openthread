#
# Copyright (c) 2020 Google LLC. All rights reserved.
#
# This document is the property of Google LLC, inc. it is considered proprietary
# and confidential information.
#
# This document may not be reproduced or transmitted in any form, in whole or in
# part, without the express written permission of Google LLC.
#

include(${PROJECT_SOURCE_DIR}/src/products/${TERBIUM_BUILD_PRODUCT}/MemoryLayout.cmake)

set(TERBIUM_NRF528XX_DIR ${PROJECT_SOURCE_DIR}/src/chips/nrf528xx)
set(NORDIC_SEMICONDUCTOR_DIR ${PROJECT_SOURCE_DIR}/../../NordicSemiconductor)

string(REPLACE "-pedantic-errors" "" OT_CFLAGS "${OT_CFLAGS}")
string(REPLACE "-Wundef" "" OT_CFLAGS "${OT_CFLAGS}")

# Build dependencies, jlink and NordicSemiconductor
set(NORDIC_SEMICONDUCTOR_DIR
    ${OPENTHREAD_ROOT_DIR}/third_party/NordicSemiconductor)

set(JLINK_DIR ${OPENTHREAD_ROOT_DIR}/third_party/jlink)

set(EXAMPLE_NRF528XX_SRC_DIR
    ${OPENTHREAD_ROOT_DIR}/examples/platforms/nrf528xx/src)

add_library(jlinkrtt OBJECT ${JLINK_DIR}/SEGGER_RTT_V640/RTT/SEGGER_RTT.c)

target_compile_definitions(
  jlinkrtt
  PRIVATE
    -DNRF52840_XXAA
    -DSEGGER_RTT_CONFIG_H=\"${NORDIC_SEMICONDUCTOR_DIR}/segger_rtt/SEGGER_RTT_Conf.h\"
    -DUSE_APP_CONFIG=1)

target_include_directories(
  jlinkrtt
  PRIVATE ${OPENTHREAD_ROOT_DIR}/include
          ${NORDIC_SEMICONDUCTOR_DIR}/cmsis
          ${NORDIC_SEMICONDUCTOR_DIR}/config
          ${NORDIC_SEMICONDUCTOR_DIR}/config/nrf52840/config
          ${NORDIC_SEMICONDUCTOR_DIR}/dependencies
          ${NORDIC_SEMICONDUCTOR_DIR}/libraries/app_error
          ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk)

list(
  APPEND
  TERBIUM_CHIP_COMMONCFLAGS
  -DNRF52840_XXAA
  -DENABLE_FEM=1
  -DUSE_APP_CONFIG=1
  -D__STARTUP_CONFIG=1)

set(COMMON_INCLUDES
    ${TERBIUM_NRF528XX_DIR}/src
    ${OPENTHREAD_ROOT_DIR}/include
    ${OPENTHREAD_ROOT_DIR}/src/core
    ${OPENTHREAD_ROOT_DIR}/third_party
    ${NORDIC_SEMICONDUCTOR_DIR}
    ${NORDIC_SEMICONDUCTOR_DIR}/config
    ${NORDIC_SEMICONDUCTOR_DIR}/cmsis
    ${NORDIC_SEMICONDUCTOR_DIR}/dependencies
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/clock
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/coex
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/fem
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/fem/three_pin_gpio
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/hal
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/raal
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/raal/softdevice
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/platform/lp_timer
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/platform/temperature
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/app_error
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/include
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/hal
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk)

set(NORDICSEMI_COMMON_SOURCES
    ${NORDIC_SEMICONDUCTOR_DIR}/dependencies/app_util_platform.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/clock/nrf_drv_clock.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/power/nrf_drv_power.c
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/app_error/app_error.c
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/app_error/app_error_weak.c
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/utf_converter/utf.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/src/nrfx_clock.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/src/nrfx_nvmc.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/src/nrfx_power.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/src/nrfx_spis.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/src/nrfx_systick.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/hal/nrf_nvmc.c
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/soc/nrfx_atomic.c)

set(NRF52840_DEFS NRF52840_XXAA NRF_802154_PROJECT_CONFIG=\"platform-config.h\")

set_property(SOURCE ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk/gcc_startup_nrf52840.S
             PROPERTY LANGUAGE C)

set(NORDICSEMI_NRF52840_SOURCES
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk/gcc_startup_nrf52840.S
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk/system_nrf52840.c)

set(NRF52840_INCLUDES
    ${OPENTHREAD_ROOT_DIR}/examples/platforms/nrf528xx/nrf52840
    ${NORDIC_SEMICONDUCTOR_DIR}/config/nrf52840/config)

set(RADIO_DRIVER_SOURCES
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/fal/nrf_802154_fal.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator/nrf_802154_ack_data.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator/nrf_802154_ack_generator.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator/nrf_802154_enh_ack_generator.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator/nrf_802154_imm_ack_generator.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/nrf_802154_csma_ca.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/nrf_802154_delayed_trx.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/nrf_802154_filter.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/nrf_802154_frame_parser.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/nrf_802154_precise_ack_timeout.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/fem/three_pin_gpio/nrf_fem_three_pin_gpio.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_core_hooks.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_core.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_critical_section.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_debug.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_pib.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_rssi.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_rx_buffer.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_timer_coord.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/platform/coex/nrf_802154_wifi_coex_none.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/platform/clock/nrf_802154_clock_ot.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/platform/hp_timer/nrf_802154_hp_timer.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/nrf_802154_rsch_crit_sect.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/nrf_802154_rsch.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/timer_scheduler/nrf_802154_timer_sched.c
)

set(TRANSPORT_SOURCES
    ${EXAMPLE_NRF528XX_SRC_DIR}/transport/spi-slave.c
    ${EXAMPLE_NRF528XX_SRC_DIR}/transport/uart.c
    ${EXAMPLE_NRF528XX_SRC_DIR}/transport/transport.c)

set(RADIO_DRIVER_SINGLE_PHY_DEFS RAAL_SINGLE_PHY=1)

set(RADIO_DRIVER_SINGLE_PHY_SOURCES
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_notification_direct.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_priority_drop_direct.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf_802154_request_direct.c
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/raal/single_phy/single_phy.c)

add_library(nordicsemi-nrf52840-sdk ${NORDICSEMI_COMMON_SOURCES}
                                    ${NORDICSEMI_NRF52840_SOURCES})

target_link_libraries(nordicsemi-nrf52840-sdk PRIVATE ot-config)

target_include_directories(
  nordicsemi-nrf52840-sdk
  PUBLIC ${OT_PUBLIC_INCLUDES} ${COMMON_INCLUDES}
  PRIVATE ${NRF52840_INCLUDES} ${TERBIUM_PRODUCT_DIR})

target_compile_definitions(
  nordicsemi-nrf52840-sdk
  PRIVATE ${NRF52840_DEFS} ${TERBIUM_PRODUCT_DEFINES}
          ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS})

target_compile_options(nordicsemi-nrf52840-sdk
                       PRIVATE ${TERBIUM_CHIP_COMMONCFLAGS})

target_link_libraries(nordicsemi-nrf52840-sdk PRIVATE ot-config)

add_library(nordicsemi-nrf52840-radio-driver ${RADIO_DRIVER_SOURCES}
                                             ${RADIO_DRIVER_SINGLE_PHY_SOURCES})

target_link_libraries(nordicsemi-nrf52840-radio-driver PRIVATE ot-config)

target_include_directories(
  nordicsemi-nrf52840-radio-driver
  PUBLIC ${COMMON_INCLUDES} ${OT_PUBLIC_INCLUDES}
  PRIVATE ${NRF52840_INCLUDES} ${TERBIUM_PRODUCT_DIR})

target_compile_definitions(
  nordicsemi-nrf52840-radio-driver
  PRIVATE ${NRF52840_DEFS} ${RADIO_DRIVER_SINGLE_PHY_DEFS}
          ${TERBIUM_PRODUCT_DEFINES})

target_compile_options(nordicsemi-nrf52840-radio-driver
                       PRIVATE ${TERBIUM_CHIP_COMMONCFLAGS})

target_link_libraries(nordicsemi-nrf52840-radio-driver PRIVATE ot-config)

add_library(openthread-nrf52840-transport
            ${TRANSPORT_SOURCES} $<TARGET_OBJECTS:openthread-platform-utils>)

target_compile_options(openthread-nrf52840-transport
                       PRIVATE ${TERBIUM_CHIP_COMMONCFLAGS})

target_include_directories(
  openthread-nrf52840-transport
  PUBLIC ${OPENTHREAD_ROOT_DIR}/examples/platforms ${COMMON_INCLUDES}
         ${OT_PUBLIC_INCLUDES} ${PROJECT_SOURCE_DIR}/src/chips/nrf528xx/src
         ${OPENTHREAD_ROOT_DIR}/examples/platforms/nrf528xx/src/transport
  PRIVATE ${NRF52840_INCLUDES} ${TERBIUM_PRODUCT_DIR})

# Build openthread-nrf52840
set(EXAMPLE_NRF528XX_SOURCES
    ${EXAMPLE_NRF528XX_SRC_DIR}/alarm.c ${EXAMPLE_NRF528XX_SRC_DIR}/entropy.c
    ${EXAMPLE_NRF528XX_SRC_DIR}/flash.c ${EXAMPLE_NRF528XX_SRC_DIR}/logging.c
    ${EXAMPLE_NRF528XX_SRC_DIR}/misc.c ${EXAMPLE_NRF528XX_SRC_DIR}/temp.c)

set(PLATFORM_SOURCES
    ${EXAMPLE_NRF528XX_SOURCES}
    ${TERBIUM_NRF528XX_DIR}/src/cmsis.c
    ${TERBIUM_NRF528XX_DIR}/src/delay.c
    ${TERBIUM_NRF528XX_DIR}/src/fault.c
    ${TERBIUM_NRF528XX_DIR}/src/flash.c
    ${TERBIUM_NRF528XX_DIR}/src/gpio.c
    ${TERBIUM_NRF528XX_DIR}/src/i2c.c
    ${TERBIUM_NRF528XX_DIR}/src/mpu.c
    ${TERBIUM_NRF528XX_DIR}/src/radio.c
    ${TERBIUM_NRF528XX_DIR}/src/radio_fem.c
    ${TERBIUM_NRF528XX_DIR}/src/system.c
    ${TERBIUM_NRF528XX_DIR}/src/timer.c
    ${TERBIUM_NRF528XX_DIR}/src/watchdog.c)

set(SINGLEPHY_SOURCES ${EXAMPLE_NRF528XX_SRC_DIR}/flash_nosd.c)

set(NRF_INCLUDES
    ${OPENTHREAD_ROOT_DIR}/include
    ${OPENTHREAD_ROOT_DIR}/examples/platforms
    ${OPENTHREAD_ROOT_DIR}/src/core
    ${TERBIUM_DIR}/src
    ${TERBIUM_DIR}/src/chips/nrf528xx/src
    ${TERBIUM_DIR}/src/products/${TERBIUM_BUILD_PRODUCT}
    ${NORDIC_SEMICONDUCTOR_DIR}
    ${NORDIC_SEMICONDUCTOR_DIR}/cmsis
    ${NORDIC_SEMICONDUCTOR_DIR}/config/nrf52840/config
    ${NORDIC_SEMICONDUCTOR_DIR}/dependencies
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/clock
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/common
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/hal
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/fem/three_pin_gpio
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/mac_features/ack_generator
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/nrf52840/platform/lp_timer
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/radio/rsch/raal
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/power
    ${NORDIC_SEMICONDUCTOR_DIR}/drivers/systick
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/app_error
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/atfifo
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/atomic
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/block_dev
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/delay
    ${NORDIC_SEMICONDUCTOR_DIR}/libraries/utf_converter
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/hal
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/drivers/include
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/mdk
    ${NORDIC_SEMICONDUCTOR_DIR}/nrfx/soc)

add_library(
  openthread-nrf52840
  ${PLATFORM_SOURCES} ${SINGLEPHY_SOURCES}
  $<TARGET_OBJECTS:openthread-platform-utils> $<TARGET_OBJECTS:jlinkrtt>)

target_link_libraries(openthread-nrf52840 PRIVATE ot-config)

target_compile_options(
  openthread-nrf52840
  PRIVATE ${OT_CFLAGS} -DCONFIG_GPIO_AS_RESET -DNRF52840_XXAA
          -DSPIS_AS_SERIAL_TRANSPORT -Wno-error=type-limits)

target_compile_definitions(
  openthread-nrf52840
  PUBLIC ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS}
         ${TERBIUM_PRODUCT_DEFINES} ${TERBIUM_CONFIG_DEFINES})

target_link_libraries(
  openthread-nrf52840
  PUBLIC terbiumplatform openthread-nrf52840-transport nordicsemi-nrf52840-sdk
         nordicsemi-nrf52840-radio-driver peripherals
  PRIVATE ot-config)

target_include_directories(
  openthread-nrf52840
  PUBLIC ${OT_PUBLIC_INCLUDES}
  PRIVATE ${NRF_INCLUDES} ${PROJECT_SOURCE_DIR}/src
          ${PROJECT_SOURCE_DIR}/src/chips/nrf528xx/src ${TERBIUM_PRODUCT_DIR})
