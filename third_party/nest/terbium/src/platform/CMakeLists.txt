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

add_definitions(-Wno-address-of-packed-member)

add_library(terbiumplatform
    backtrace.cpp
    boot.cpp
    coex.cpp
    diag.cpp
    diag_peripheral.cpp
    diag_sysenv.cpp
    diag_wireless_cal.cpp
    power_manager.cpp
    power_settings.cpp
    regulatory_domain.cpp
    sysenv.cpp
    wireless_cal.cpp
)

target_link_libraries(terbiumplatform PRIVATE ot-config)

target_include_directories(terbiumplatform
    PRIVATE
    ${OPENTHREAD_ROOT_DIR}/examples/platforms
    ${OPENTHREAD_ROOT_DIR}/include
    ${OPENTHREAD_ROOT_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src
    ${TERBIUM_PRODUCT_DIR}
)

target_compile_definitions(terbiumplatform
    PRIVATE
    ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS}
    ${TERBIUM_CONFIG_DEFINES}
)

target_link_libraries(terbiumplatform
    PUBLIC
    ${OT_PLATFORM_LIB}
)
