#
#  Copyright (c) 2020 Google LLC.
#  All rights reserved.
#
#  This document is the property of Google LLC, inc. it is
#  considered proprietary and confidential information.
#
#  This document may not be reproduced or transmitted in any form,
#  in whole or in part, without the express written permission of
#  Google LLC.
#

add_library(peripherals
    fem_sim.c
    sky66403.c
    sky66408.c
    tlv75801.c
)

target_link_libraries(peripherals
    PRIVATE
    ${OT_PLATFORM_LIB}
)

target_include_directories(peripherals
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${OPENTHREAD_ROOT_DIR}/examples/platforms
    ${OPENTHREAD_ROOT_DIR}/include
    ${TERBIUM_PRODUCT_DIR}
)
