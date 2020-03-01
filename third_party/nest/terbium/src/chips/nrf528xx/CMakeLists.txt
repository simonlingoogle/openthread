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

if(TERBIUM_CHIP STREQUAL "nrf52811")
add_subdirectory(nrf52811)
endif()

if(TERBIUM_CHIP STREQUAL "nrf52840")
add_subdirectory(nrf52840)
endif()

set(TERBIUM_MEMORY_LAYOUT_DEFINITIONS ${TERBIUM_MEMORY_LAYOUT_DEFINITIONS} PARENT_SCOPE)
set(TERBIUM_MEMORY_SIGNATURE_SECTION ${TERBIUM_MEMORY_SIGNATURE_SECTION} PARENT_SCOPE)

