/*
 *  Copyright (c) 2020 Google LLC.
 *  All rights reserved.
 *
 *  This document is the property of Google LLC, Inc. It is
 *  considered proprietary and confidential information.
 *
 *  This document may not be reproduced or transmitted in any form,
 *  in whole or in part, without the express written permission of
 *  Google LLC.
 *
 */

/**
 * @file
 *   GCC linker script for bootloader.
 */

SEARCH_DIR(.)

GROUP(-lgcc -lc -lnosys)

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(Reset_Handler)

SECTIONS
{
    . = TERBIUM_MEMORY_BOOTLOADER_BASE_ADDRESS;
    .text :
    {
        PROVIDE(__TEXT__start = .);
        PROVIDE(__VECTOR__table = .);
        KEEP(*(.isr_vector))

        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)

        KEEP(*(.eh_frame*))

        PROVIDE(__TEXT__end = .);
    }
    PROVIDE(__TEXT__size = SIZEOF(.text));

    __etext = .;

    . = TERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS;
    .preserve (NOLOAD) :
    {
        PROVIDE(__PRESERVE__start__ = .);
        KEEP(*(.preserve .preserve.*))
        PROVIDE(__PRESERVE__end__ = .);
    }
    PROVIDE(__PRESERVE__size = SIZEOF(.preserve));

    .data : AT (__etext)
    {
        __data_start__ = .;
        *(vtable)
        *(.data*)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;
    }

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    }

    .heap (NOLOAD) :
    {
        __HeapBase = .;
        __end__ = .;
        PROVIDE(end = .);
        KEEP(*(.heap*))
        __HeapLimit = .;
    }

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (NOLOAD):
    {
        KEEP(*(.stack*))
    }

    /* Remove unwind tables.  We don't use C++ exception handling. */
    /DISCARD/ :
    {
        * (.ARM.exidx);
    }

    /* Assure that code does not overlap flash data area.*/
    ASSERT((__etext + SIZEOF(.data) <= TERBIUM_MEMORY_BOOTLOADER_TOP_ADDRESS), "Error: Code overlaps flash data area.")

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS - 1;
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
