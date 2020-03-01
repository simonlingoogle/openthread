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

include(${PROJECT_SOURCE_DIR}/src/chips/nrf528xx/nrf52840/MemoryLayout.cmake)

set(TERBIUM_CHIP "nrf52840")
set(OPENTHREAD_PROJECT_CORE_CONFIG_FILE "openthread-core-project-config.h")
set(OT_PLATFORM_LIB openthread-nrf52840)
list(APPEND TERBIUM_CHIP_COMMONCFLAGS -DSPIS_AS_SERIAL_TRANSPORT=1)
list(APPEND OT_CFLAGS -DOPENTHREAD_CONFIG_NCP_SPI_ENABLE=1)
