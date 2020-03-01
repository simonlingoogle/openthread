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

set(CMAKE_SYSTEM_NAME              Generic)
set(CMAKE_SYSTEM_PROCESSOR         ARM)

set(CMAKE_C_COMPILER               arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER             arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER             arm-none-eabi-as)
set(CMAKE_RANLIB                   arm-none-eabi-ranlib)
set(CMAKE_OBJCOPY                  arm-none-eabi-objcopy)

set(COMMON_C_FLAGS                 "-Os -mcpu=cortex-m4 -mfloat-abi=soft -mfpu=fpv4-sp-d16 -mthumb -mabi=aapcs -fdata-sections -ffunction-sections")
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7)
    set(CMAKE_C_FLAGS                  "${COMMON_C_FLAGS} -Wno-expansion-to-defined")
endif()
set(CMAKE_C_FLAGS                  "${COMMON_C_FLAGS} -std=gnu99")
set(CMAKE_CXX_FLAGS                "${COMMON_C_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS                "${COMMON_C_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS         "${COMMON_C_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

set(CMAKE_C_FLAGS_DEBUG            "-Os -g")
set(CMAKE_CXX_FLAGS_DEBUG          "-Os -g")
set(CMAKE_ASM_FLAGS_DEBUG          "-Os -g")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG   "-Os -g")

set(CMAKE_C_FLAGS_RELEASE          "-Os")
set(CMAKE_CXX_FLAGS_RELEASE        "-Os")
set(CMAKE_ASM_FLAGS_RELEASE        "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
