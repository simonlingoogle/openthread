#
#  Copyright (c) 2020 Google LLC.
#  All rights reserved.
#
#  This document is the property of Google LLC, Inc. It is
#  considered proprietary and confidential information.
#
#  This document may not be reproduced or transmitted in any form,
#  in whole or in part, without the express written permission of
#  Google LLC.
#

include(${PROJECT_SOURCE_DIR}/src/chips/nrf528xx/nrf52811/MemoryLayout.cmake)

set(OT_MBEDTLS_CONFIG_FILE \"mbedtls-config.h\" CACHE STRING "The mbedTLS config file")
set(MBEDTLS_DEFINES
    -DMBEDTLS_USER_CONFIG_FILE=\"nrf52811-mbedtls-config.h\"
    -DNRF52811_XXAA
    -DDISABLE_CC310=1
    CACHE
    STRING
    "The mbedTLS defines"
)
set(MBEDTLS_INCLUDES
    ${OPENTHREAD_ROOT_DIR}/third_party/NordicSemiconductor/libraries/nrf_security/mbedtls_plat_config
    CACHE
    STRING
    "The mbedTLS includes"
)

set(OPENTHREAD_EXAMPLE_CLI_DIR ${OPENTHREAD_ROOT_DIR}/examples/apps/cli)

add_executable(ot-cli-mtd
    ${OPENTHREAD_EXAMPLE_CLI_DIR}/main.c
)

set(COMMON_INCLUDES
    ${OT_PUBLIC_INCLUDES}
    ${OT_PRIVATE_INCLUDES}
    ${OPENTHREAD_ROOT_DIR}/examples/platforms
    ${OPENTHREAD_ROOT_DIR}/src/core
)

target_compile_definitions(ot-cli-mtd
    PRIVATE
    ${TERBIUM_CONFIG_DEFINES}
)

target_include_directories(ot-cli-mtd PRIVATE ${COMMON_INCLUDES})

target_link_libraries(ot-cli-mtd
    openthread-cli-mtd
    ${OT_PLATFORM_LIB}
    openthread-mtd
    ${OT_PLATFORM_LIB}
    mbedcrypto
)

target_link_options(ot-cli-mtd PRIVATE -T${PROJECT_BINARY_DIR}/${AppLinkerScript})

add_custom_command(TARGET ot-cli-mtd
    POST_BUILD
    COMMAND /bin/bash ${PROJECT_SOURCE_DIR}/scripts/copy-and-sign ${PROJECT_SOURCE_DIR} ${OUTPUT_ROOT_DIR} ${TERBIUM_MEMORY_SIGNATURE_SECTION} ${CMAKE_OBJCOPY}
    COMMENT Add signature for the images
)

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
