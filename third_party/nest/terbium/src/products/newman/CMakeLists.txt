#
# Copyright (c) 2020 Google LLC. All rights reserved.
#
# This document is the property of Google LLC, Inc. It is considered proprietary
# and confidential information.
#
# This document may not be reproduced or transmitted in any form, in whole or in
# part, without the express written permission of Google LLC.
#

include(${PROJECT_SOURCE_DIR}/src/chips/nrf528xx/nrf52840/MemoryLayout.cmake)

set(OT_MBEDTLS_CONFIG_FILE
    \"mbedtls-config.h\"
    CACHE STRING "The mbedTLS config file")

list(APPEND TERBIUM_PRODUCT_DEFINES
     "TERBIUM_CONFIG_FEM_PIN_CONTROLLER_ENABLE=1")

set(TERBIUM_PRODUCT_DEFINES
    ${TERBIUM_PRODUCT_DEFINES}
    PARENT_SCOPE)

set(OPENTHREAD_EXAMPLE_NCP_DIR ${OPENTHREAD_ROOT_DIR}/examples/apps/ncp)

add_executable(ot-rcp ${OPENTHREAD_EXAMPLE_NCP_DIR}/main.c)

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES} ${OT_PRIVATE_INCLUDES}
    ${OPENTHREAD_ROOT_DIR}/examples/platforms ${OPENTHREAD_ROOT_DIR}/src/core)

target_compile_definitions(ot-rcp PRIVATE ${TERBIUM_CONFIG_DEFINES})

target_include_directories(ot-rcp PRIVATE ${COMMON_INCLUDES})

target_link_libraries(ot-rcp openthread-rcp ${OT_PLATFORM_LIB} openthread-radio
                      ${OT_PLATFORM_LIB} mbedcrypto)

target_link_options(ot-rcp PRIVATE -T${PROJECT_BINARY_DIR}/${AppLinkerScript})

add_custom_command(
  TARGET ot-rcp
  POST_BUILD
  COMMAND
    /bin/bash ${PROJECT_SOURCE_DIR}/scripts/copy-and-sign ${PROJECT_SOURCE_DIR}
    ${OUTPUT_ROOT_DIR} ${TERBIUM_MEMORY_SIGNATURE_SECTION} ${CMAKE_OBJCOPY}
  COMMENT Add signature for the images)

add_executable(bootloader
    ${PROJECT_SOURCE_DIR}/app/bootloader/main.c
)

target_link_libraries(bootloader
    PRIVATE
    ${OT_PLATFORM_LIB}
)

target_compile_definitions(bootloader
    PRIVATE
    ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS}
)

target_link_options(bootloader PRIVATE -T${PROJECT_BINARY_DIR}/${BootloaderLinkerScript})
