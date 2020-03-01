#
#   Copyright (c) 2020 Google LLC.
#   All rights reserved.
#
#   This document is the property of Google LLC, Inc. It is
#   considered proprietary and confidential information.
#
#   This document may not be reproduced or transmitted in any form,
#   in whole or in part, without the express written permission of
#   Google LLC.
#

cmake_minimum_required(VERSION 3.14)
project(terbium)

include(${PROJECT_SOURCE_DIR}/etc/cmake/functions.cmake)

set(OPENTHREAD_ROOT_DIR ${PROJECT_SOURCE_DIR}/../../..)

set(OT_CFLAGS
    $<$<COMPILE_LANGUAGE:C>:${OT_CFLAGS} -Wall -Wextra -Wshadow -Werror -pedantic-errors>
    $<$<COMPILE_LANGUAGE:CXX>:${OT_FLAGS} -Wall -Wextra -Wshadow -Wno-c++14-compat -fno-exceptions>
)

add_library(ot-config INTERFACE)

# Generate openthread-config-generic.h
configure_file(${OPENTHREAD_ROOT_DIR}/etc/cmake/openthread-config-generic.h.in ${PROJECT_BINARY_DIR}/etc/cmake/openthread-config-generic.h)
list(APPEND OT_PUBLIC_INCLUDES ${PROJECT_BINARY_DIR}/etc/cmake)

list(APPEND OT_PUBLIC_INCLUDES ${OPENTHREAD_ROOT_DIR}/include)

# Set PACKAGE_NAME and PACKAGE_VERSION
file(READ ${OPENTHREAD_ROOT_DIR}/.default-version OT_DEFAULT_VERSION)
execute_process(
    COMMAND bash "-c" "third_party/nlbuild-autotools/repo/scripts/mkversion -b ${OT_DEFAULT_VERSION}"
    WORKING_DIRECTORY ${OPENTHREAD_ROOT_DIR}
    OUTPUT_VARIABLE OT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Version: ${OT_VERSION}")

target_compile_definitions(ot-config INTERFACE "PACKAGE_NAME=\"OPENTHREAD\"")
target_compile_definitions(ot-config INTERFACE "PACKAGE_VERSION=\"${OT_VERSION}\"")

# mbedtls
set(OT_EXTERNAL_MBEDTLS "" CACHE STRING "Specify external mbedtls library")
set(OT_MBEDTLS mbedtls)
if(NOT OT_EXTERNAL_MBEDTLS)
    list(APPEND OT_PRIVATE_INCLUDES ${OPENTHREAD_ROOT_DIR}/third_party/mbedtls)
    list(APPEND OT_PRIVATE_INCLUDES ${OPENTHREAD_ROOT_DIR}/third_party/mbedtls/repo/include)
endif()

# Terbium build product
terbium_get_products(TERBIUM_PRODUCTS)
if(NOT TERBIUM_BUILD_PRODUCT IN_LIST TERBIUM_PRODUCTS)
    message(FATAL_ERROR "Unknown Terbium product: ${TERBIUM_BUILD_PRODUCT}")
endif()
message(STATUS "Terbium build product: ${TERBIUM_BUILD_PRODUCT}")

# Terbium build variant
message(STATUS "Terbium build variant: ${TERBIUM_BUILD_VARIANT}")
if(TERBIUM_BUILD_VARIANT STREQUAL "eng")
    list(APPEND TERBIUM_CONFIG_DEFINES -DTERBIUM_BUILD_ENG=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
elseif(TERBIUM_BUILD_VARIANT STREQUAL "user")
    list(APPEND TERBIUM_CONFIG_DEFINES -DTERBIUM_BUILD_USER=1)
else()
    message(FATAL_ERROR "Unknown TERBIUM_BUILD_VARIANT:${TERBIUM_BUILD_VARIANT}")
endif()
list(APPEND TERBIUM_CONFIG_DEFINES -DTERBIUM_BUILD_VARIANT="${TERBIUM_BUILD_VARIANT}")

# Output root directory
set(OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/output/${TERBIUM_BUILD_PRODUCT}/${TERBIUM_BUILD_VARIANT})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_ROOT_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_ROOT_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_ROOT_DIR}/bin)

set(TERBIUM_PRODUCT_DIR ${PROJECT_SOURCE_DIR}/src/products/${TERBIUM_BUILD_PRODUCT})
include(${TERBIUM_PRODUCT_DIR}/product.cmake)

# Set openthread core config file
message(STATUS "Project core config: \"${OPENTHREAD_PROJECT_CORE_CONFIG_FILE}\"")
target_compile_definitions(ot-config INTERFACE "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OPENTHREAD_PROJECT_CORE_CONFIG_FILE}\"")

add_subdirectory(src)

target_compile_definitions(ot-config INTERFACE ${TERBIUM_CONFIG_DEFINES})
target_compile_definitions(ot-config INTERFACE ${MBEDTLS_DEFINES})

list(APPEND OT_PRIVATE_INCLUDES ${MBEDTLS_INCLUDES})

# Add the include path for the openthread core config file
list(APPEND OT_PRIVATE_INCLUDES ${TERBIUM_PRODUCT_DIR})

# Set this for outside openthread repo build as if it were the root source directory
set(PROJECT_SOURCE_DIR ${OPENTHREAD_ROOT_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/openthread)
add_subdirectory(${PROJECT_SOURCE_DIR}/examples ${PROJECT_BINARY_DIR}/examples)
add_subdirectory(${PROJECT_SOURCE_DIR}/third_party ${PROJECT_BINARY_DIR}/third_party)
