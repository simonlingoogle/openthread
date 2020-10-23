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

set(TERBIUM_CHIP "nrf52840")
set(OPENTHREAD_PROJECT_CORE_CONFIG_FILE "openthread-core-project-config.h")
set(OT_PLATFORM_LIB openthread-nrf52840)
set(OT_RCP ON)
set(OT_FTD ON)
list(APPEND TERBIUM_CHIP_COMMONCFLAGS -DUART_AS_SERIAL_TRANSPORT=1)
list(APPEND OT_CFLAGS -DOPENTHREAD_CONFIG_NCP_UART_ENABLE=1)
