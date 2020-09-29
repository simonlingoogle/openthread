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

# Memory Layout:
#
#       / +------------------+ <--0x20040000 \
#       | |                  |               |
#       | |       stack      |               | Minimum Stack Size
#       | |..................| <--0x2003F000 /
#       | |                  |
#       | +------------------+
#       | |     stack guard  |
#       | +------------------+
#       | |       heap       |
#       | +------------------+
#  RAM  | |       bss        |
#       | +------------------+
#       | |                  |
#       | |       data       |
#       | |                  |
#       | +------------------+ <--0x20000040
#       | |       guard      |
#       | +------------------+ <--0x20000020
#       | |      preserve    |
#       \ +------------------+ <--0x20000000
#
#       / +------------------+ <--0x00100000
#       | |        env       |
#       | +------------------+ <--0x000FE000
#       | |                  |
#       | |                  |
#       | +------------------+
#       | |    |  signature  |
#       | |    |-------------+
# Flash | |    |             |
#       | |app |  text       |
#       | |    |             |
#       | |    |-------------+ <--0x0000500C
#       | |    |  aat        |
#       | +------------------+ <--0x00005000
#       | |      sysenv      |
#       | +------------------+ <--0x00004000
#       | |                  |
#       | |     bootloader   |
#       | |                  |
#       \ +------------------+ <--0x00000000

# Flash
set(TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS 0x00000000)
set(TERBIUM_MEMORY_SOC_FLASH_SIZE         0x100000)
math(EXPR TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS "${TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS} + ${TERBIUM_MEMORY_SOC_FLASH_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# RAM
set(TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS 0x20000000)
set(TERBIUM_MEMORY_SOC_DATA_RAM_SIZE         0x40000)
math(EXPR TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS "${TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS} + ${TERBIUM_MEMORY_SOC_DATA_RAM_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# Bootloader
set(TERBIUM_MEMORY_BOOTLOADER_BASE_ADDRESS ${TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS})
set(TERBIUM_MEMORY_BOOTLOADER_SIZE         0x4000)
math(EXPR TERBIUM_MEMORY_BOOTLOADER_TOP_ADDRESS "${TERBIUM_MEMORY_BOOTLOADER_BASE_ADDRESS} + ${TERBIUM_MEMORY_BOOTLOADER_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# SysEnv
# Internal flash erase size is 4KB, so sysenv has to be minimum of 4KB.
set(TERBIUM_MEMORY_SYSENV_BASE_ADDRESS ${TERBIUM_MEMORY_BOOTLOADER_TOP_ADDRESS})
set(TERBIUM_MEMORY_SYSENV_SIZE         0x1000)
math(EXPR TERBIUM_MEMORY_SYSENV_TOP_ADDRESS "${TERBIUM_MEMORY_SYSENV_BASE_ADDRESS} + ${TERBIUM_MEMORY_SYSENV_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# Application
set(TERBIUM_MEMORY_ENV_SIZE 0x2000)
math(EXPR TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS "${TERBIUM_MEMORY_SYSENV_BASE_ADDRESS} + ${TERBIUM_MEMORY_SYSENV_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR TERBIUM_MEMORY_NON_APPLICATION_SIZE "${TERBIUM_MEMORY_BOOTLOADER_SIZE} + ${TERBIUM_MEMORY_ENV_SIZE} + ${TERBIUM_MEMORY_SYSENV_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR TERBIUM_MEMORY_APPLICATION_SIZE "${TERBIUM_MEMORY_SOC_FLASH_SIZE} - ${TERBIUM_MEMORY_NON_APPLICATION_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR TERBIUM_MEMORY_APPLICATION_TOP_ADDRESS "${TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS} + ${TERBIUM_MEMORY_APPLICATION_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
set(TERBIUM_MEMORY_APPLICATION_RAM_BASE_ADDRESS ${TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS})
set(TERBIUM_MEMORY_APPLICATION_RAM_TOP_ADDRESS ${TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS})

# Preserve
# Preserved RAM section is used to track boot counts for identifying boot loops.
set(TERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS ${TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS})
set(TERBIUM_MEMORY_PRESERVE_RAM_MAX_SIZE 0x20)

# Guard
# Marble bootloader uses the Guard section to guard the CSTACK. When Marble
# bootloader boots up, it calls the MPU to set Guard section in RAM to
# read-only. The Guard Region is under CSTACK. If CSTACK is overflow, CPU will
# write data to Guard Region section, which triger an exception.
math(EXPR TERBIUM_MEMORY_GUARD_RAM_BASE_ADDRESS "${TERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS} + ${TERBIUM_MEMORY_PRESERVE_RAM_MAX_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
set(TERBIUM_MEMORY_GUARD_RAM_SIZE 0x20)

# Signature
# Signature section is used for secure boot verification. The start address of
# signature section is also the address of the end of the image since the
# signature is placed by linker script at the end.
set(TERBIUM_MEMORY_SIGNATURE_SECTION .signature)
set(TERBIUM_MEMORY_SIGNATURE_SIZE 0x3c)

# Env
math(EXPR TERBIUM_MEMORY_ENV_BASE_ADDRESS "${TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS} + ${TERBIUM_MEMORY_APPLICATION_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR TERBIUM_MEMORY_ENV_TOP_ADDRESS "${TERBIUM_MEMORY_ENV_BASE_ADDRESS} + ${TERBIUM_MEMORY_ENV_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

# Stack
# Here defines the minimum stack size needed by the program. The actual available
# stack size is determined by the end address of the Stack Guard section.
set(TERBIUM_MEMORY_MIN_STACK_SIZE 0x1000)

# Satck Guard
# Stack Guard section is placed above the heap. When the stack overflows, MPU
# will generates a hard fault to capture the fault.
set(TERBIUM_MEMORY_STACK_GUARD_SIZE 0x100)

# Heap
# OpenThread calls the function 'srand()' to initialize the entropy. The function
# `srand()` calls `malloc()` to alloc at most 256 bytes space.
set(TERBIUM_MEMORY_HEAP_SIZE 0x100)

set(TERBIUM_MEMORY_LAYOUT_DEFINITIONS
    -DTERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS=${TERBIUM_MEMORY_SOC_FLASH_BASE_ADDRESS}
    -DTERBIUM_MEMORY_SOC_FLASH_SIZE=${TERBIUM_MEMORY_SOC_FLASH_SIZE}
    -DTERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS=${TERBIUM_MEMORY_SOC_FLASH_TOP_ADDRESS}
    -DTERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS=${TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS}
    -DTERBIUM_MEMORY_SOC_DATA_RAM_SIZE=${TERBIUM_MEMORY_SOC_DATA_RAM_SIZE}
    -DTERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS=${TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS}
    -DTERBIUM_MEMORY_BOOTLOADER_BASE_ADDRESS=${TERBIUM_MEMORY_BOOTLOADER_BASE_ADDRESS}
    -DTERBIUM_MEMORY_BOOTLOADER_SIZE=${TERBIUM_MEMORY_BOOTLOADER_SIZE}
    -DTERBIUM_MEMORY_BOOTLOADER_TOP_ADDRESS=${TERBIUM_MEMORY_BOOTLOADER_TOP_ADDRESS}
    -DTERBIUM_MEMORY_SYSENV_BASE_ADDRESS=${TERBIUM_MEMORY_SYSENV_BASE_ADDRESS}
    -DTERBIUM_MEMORY_SYSENV_SIZE=${TERBIUM_MEMORY_SYSENV_SIZE}
    -DTERBIUM_MEMORY_SYSENV_TOP_ADDRESS=${TERBIUM_MEMORY_SYSENV_TOP_ADDRESS}
    -DTERBIUM_MEMORY_APPLICATION_BASE_ADDRESS=${TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS}
    -DTERBIUM_MEMORY_APPLICATION_SIZE=${TERBIUM_MEMORY_APPLICATION_SIZE}
    -DTERBIUM_MEMORY_APPLICATION_RAM_BASE_ADDRESS=${TERBIUM_MEMORY_APPLICATION_RAM_BASE_ADDRESS}
    -DTERBIUM_MEMORY_APPLICATION_TOP_ADDRESS=${TERBIUM_MEMORY_APPLICATION_TOP_ADDRESS}
    -DTERBIUM_MEMORY_APPLICATION_RAM_TOP_ADDRESS=${TERBIUM_MEMORY_APPLICATION_RAM_TOP_ADDRESS}
    -DTERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS=${TERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS}
    -DTERBIUM_MEMORY_PRESERVE_RAM_MAX_SIZE=${TERBIUM_MEMORY_PRESERVE_RAM_MAX_SIZE}
    -DTERBIUM_MEMORY_GUARD_RAM_BASE_ADDRESS=${TERBIUM_MEMORY_GUARD_RAM_BASE_ADDRESS}
    -DTERBIUM_MEMORY_GUARD_RAM_SIZE=${TERBIUM_MEMORY_GUARD_RAM_SIZE}
    -DTERBIUM_MEMORY_SIGNATURE_SECTION=${TERBIUM_MEMORY_SIGNATURE_SECTION}
    -DTERBIUM_MEMORY_SIGNATURE_SIZE=${TERBIUM_MEMORY_SIGNATURE_SIZE}
    -DTERBIUM_MEMORY_ENV_BASE_ADDRESS=${TERBIUM_MEMORY_ENV_BASE_ADDRESS}
    -DTERBIUM_MEMORY_ENV_SIZE=${TERBIUM_MEMORY_ENV_SIZE}
    -DTERBIUM_MEMORY_ENV_TOP_ADDRESS=${TERBIUM_MEMORY_ENV_TOP_ADDRESS}
    -DTERBIUM_MEMORY_MIN_STACK_SIZE=${TERBIUM_MEMORY_MIN_STACK_SIZE}
    -DTERBIUM_MEMORY_STACK_GUARD_SIZE=${TERBIUM_MEMORY_STACK_GUARD_SIZE}
    -DTERBIUM_MEMORY_HEAP_SIZE=${TERBIUM_MEMORY_HEAP_SIZE}
)

set(AppLinkerScript src/products/nrf52840dk/app.ld)
set(BootloaderLinkerScript src/products/nrf52840dk/bootloader.ld)
