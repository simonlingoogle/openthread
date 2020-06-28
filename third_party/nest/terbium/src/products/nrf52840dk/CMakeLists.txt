#
# Copyright (c) 2020 Google LLC. All rights reserved.
#
# This document is the property of Google LLC, Inc. It is considered proprietary
# and confidential information.
#
# This document may not be reproduced or transmitted in any form, in whole or in
# part, without the express written permission of Google LLC.
#

include(MemoryLayout.cmake)

# Generate linker script
string(REPLACE ";" " " TERBIUM_MEMORY_LAYOUT_DEFINITIONS_STR
               "${TERBIUM_MEMORY_LAYOUT_DEFINITIONS}")

execute_process(
  COMMAND
      bash "-c"
      "${CMAKE_C_COMPILER} -E -C ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS_STR} -P - <${CMAKE_CURRENT_SOURCE_DIR}/app.ld> ${PROJECT_BINARY_DIR}/src/products/nrf52840dk/app.ld"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

execute_process(
  COMMAND
      bash "-c"
      "${CMAKE_C_COMPILER} -E -C ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS_STR} -P - <${CMAKE_CURRENT_SOURCE_DIR}/bootloader.ld> ${PROJECT_BINARY_DIR}/src/products/nrf52840dk/bootloader.ld"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

set(TERBIUM_MEMORY_LAYOUT_DEFINITIONS
    ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS}
    PARENT_SCOPE)

set(TERBIUM_MEMORY_SIGNATURE_SECTION
    ${TERBIUM_MEMORY_SIGNATURE_SECTION}
    PARENT_SCOPE)

set(OT_MBEDTLS_CONFIG_FILE
    \"mbedtls-config.h\"
    CACHE STRING "The mbedTLS config file")

set(TERBIUM_PRODUCT_DEFINES
    ${TERBIUM_PRODUCT_DEFINES}
    PARENT_SCOPE)

set(OPENTHREAD_EXAMPLE_NCP_DIR ${OPENTHREAD_ROOT_DIR}/examples/apps/ncp)

add_executable(ot-rcp ${OPENTHREAD_EXAMPLE_NCP_DIR}/main.c)

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES}
    ${OPENTHREAD_ROOT_DIR}/examples/platforms ${OPENTHREAD_ROOT_DIR}/src/core)

target_compile_definitions(ot-rcp PRIVATE ${TERBIUM_CONFIG_DEFINES})

target_include_directories(ot-rcp PRIVATE ${COMMON_INCLUDES})

target_link_libraries(ot-rcp openthread-rcp ${OT_PLATFORM_LIB} openthread-radio
                      ${OT_PLATFORM_LIB} mbedcrypto ot-config)

target_link_options(ot-rcp PRIVATE -T${PROJECT_BINARY_DIR}/${AppLinkerScript})

add_custom_command(
    TARGET ot-rcp
    POST_BUILD
    COMMAND
        /bin/bash ${PROJECT_SOURCE_DIR}/scripts/copy-and-sign ${PROJECT_SOURCE_DIR}
        ${OUTPUT_ROOT_DIR} ${TERBIUM_MEMORY_SIGNATURE_SECTION} ${CMAKE_OBJCOPY} ot-rcp
    COMMENT Add signature for the images)

set(OPENTHREAD_EXAMPLE_CLI_DIR ${OPENTHREAD_ROOT_DIR}/examples/apps/cli)

add_executable(ot-cli-ftd
    ${OPENTHREAD_EXAMPLE_CLI_DIR}/main.c
)

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES}
    ${OPENTHREAD_ROOT_DIR}/examples/platforms
    ${OPENTHREAD_ROOT_DIR}/src/core
)

target_compile_definitions(ot-cli-ftd
    PRIVATE
    ${TERBIUM_CONFIG_DEFINES}
)

target_include_directories(ot-cli-ftd PRIVATE ${COMMON_INCLUDES})

target_link_libraries(ot-cli-ftd
    openthread-cli-ftd
    ${OT_PLATFORM_LIB}
    openthread-ftd
    ${OT_PLATFORM_LIB}
    mbedcrypto
    ot-config
)

target_link_options(ot-cli-ftd PRIVATE -T${PROJECT_BINARY_DIR}/${AppLinkerScript})

add_custom_command(
    TARGET ot-cli-ftd
    POST_BUILD
    COMMAND
        /bin/bash ${PROJECT_SOURCE_DIR}/scripts/copy-and-sign ${PROJECT_SOURCE_DIR}
        ${OUTPUT_ROOT_DIR} ${TERBIUM_MEMORY_SIGNATURE_SECTION} ${CMAKE_OBJCOPY} ot-cli-ftd
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

add_custom_command(
    TARGET bootloader
    POST_BUILD
    COMMAND
        /bin/bash ${PROJECT_SOURCE_DIR}/scripts/copy-and-sign ${PROJECT_SOURCE_DIR}
        ${OUTPUT_ROOT_DIR} ${TERBIUM_MEMORY_SIGNATURE_SECTION} ${CMAKE_OBJCOPY} bootloader
    COMMENT Add signature for the images)
