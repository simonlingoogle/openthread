/**
  * Copyright (c) 2020 Google LLC.
  * All rights reserved.
  *
  * This document is the property of Google LLC, inc. it is
  * considered proprietary and confidential information.
  *
  * This document may not be reproduced or transmitted in any form,
  * in whole or in part, without the express written permission of
  * Google LLC.
  */

/**
 * @file
 *   GCC linker script for application.
 */

SEARCH_DIR(.)

GROUP(-lgcc -lc -lnosys)

OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

ENTRY(Reset_Handler)

SECTIONS
{
    . = TERBIUM_MEMORY_SYSENV_BASE_ADDRESS;
    .sysenv (NOLOAD) :
    {
        . = ALIGN(4);
        PROVIDE(__SYSENV__start = .);
        KEEP(*(.sysenv .sysenv.*));
        PROVIDE(__SYSENV__end = .);
    }
    PROVIDE(__SYSENV__size = SIZEOF(.sysenv));

    . = TERBIUM_MEMORY_APPLICATION_BASE_ADDRESS;
    .aat :
    {
        . = ALIGN(4);
        PROVIDE(__AAT__start = .);
        KEEP(*(.aat .aat.*));
        PROVIDE(__AAT__end = .);
    }
    PROVIDE(__AAT__size = SIZEOF(.aat));

    .text :
    {
       /*
        * The Vector table must be naturally aligned to a power of two whose
        * alignment value is greater than or equal to the vector table size.
        * Ref: https://developer.arm.com/docs/100701/0200/exception-properties
        *
        * The vector table (section `.isr_vector`) size is 127 words. The
        * minimum alignment is set to 128 bytes. When tested, we found that
        * if the alignment is set to 128 bytes, the RTC1 interrupt doesn't
        * work. So the alignment is set to 256 bytes.
        */
        . = ALIGN(256);
        PROVIDE(__VECTOR__table = .);
        PROVIDE(__TEXT__start = .);
        KEEP(*(.isr_vector));
        *(.text*);

        KEEP(*(.init));
        KEEP(*(.fini));

        /* .ctors */
        *crtbegin.o(.ctors);
        *crtbegin?.o(.ctors);
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors);
        *(SORT(.ctors.*));
        *(.ctors);

        /* .dtors */
        *crtbegin.o(.dtors);
        *crtbegin?.o(.dtors);
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors);
        *(SORT(.dtors.*));
        *(.dtors);

        *(.rodata*);

        KEEP(*(.eh_frame*));

        PROVIDE(__TEXT__end = .);
    }
    PROVIDE(__TEXT__size = SIZEOF(.text));

    __etext = .;

    . = TERBIUM_MEMORY_ENV_BASE_ADDRESS;
    .env (NOLOAD) :
    {
        . = ALIGN(4);
        PROVIDE(__ENV__start = .);
        KEEP(*(.env .env.*));
        PROVIDE(__ENV__end = .);
    }
    PROVIDE(__ENV__size = SIZEOF(.env));

    . = TERBIUM_MEMORY_PRESERVE_RAM_BASE_ADDRESS;
    .preserve (NOLOAD) :
    {
        PROVIDE(__PRESERVE__start__ = .);
        KEEP(*(.preserve .preserve.*));
        PROVIDE(__PRESERVE__end__ = .);
    }
    PROVIDE(__PRESERVE__size = SIZEOF(.preserve));

    . = TERBIUM_MEMORY_GUARD_RAM_BASE_ADDRESS;
    .guard (NOLOAD) :
    {
        . = ALIGN(32);
        PROVIDE(__GUARD_start__ = .);
        KEEP(*(.guard .guard.*));
        PROVIDE(__GUARD_end__ = .);
    }
    PROVIDE(__GUARD__size = SIZEOF(.guard));

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;
        *(vtable);
        *(.data*);

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array));
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array));
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)));
        KEEP(*(.fini_array));
        PROVIDE_HIDDEN (__fini_array_end = .);

        KEEP(*(.jcr*));
        . = ALIGN(4);

        /* All data end */
        __data_end__ = .;
    }

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*);
        *(COMMON);
        . = ALIGN(4);
        __bss_end__ = .;
    }

    . = ALIGN(4);
    .heap (NOLOAD) :
    {
        __HeapBase = .;
        __end__ = .;
        PROVIDE(end = .);
        KEEP(*(.heap*))
        __HeapLimit = .;
    }

    . = ALIGN(32);
    .sguard (NOLOAD) :
    {
        PROVIDE(__STACKGUARD_start = .);
        KEEP(*(.sguard*));
        . = __STACKGUARD_start + TERBIUM_MEMORY_STACK_GUARD_SIZE;
        PROVIDE(__STACKGUARD_end = .);
    }

    /* ".stack_dummy" section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later
     */
    . = ALIGN(4);
    .stack_dummy (NOLOAD) :
    {
        PROVIDE(__STACK__start = .);
        KEEP(*(.stack*));
        PROVIDE(__STACK__end = .);
    }

    /* Remove unwind tables.  We don't use C++ exception handling. */
    /DISCARD/ :
    {
        *(.ARM.exidx);
    }

    /* Add an empty SIGNATURE_SECTION section to the end of the image.
     * The signing tool will replace it with the computed signature.
     * The first byte of the signature block has to be initialized
     * to zero, indicating no signature is present.
     */
    . = __etext + SIZEOF(.data);
    TERBIUM_MEMORY_SIGNATURE_SECTION : AT (__etext + SIZEOF(.data))
    {
        . = ALIGN(4);
        PROVIDE(__SIGNATURE__start = .);
        KEEP(*(TERBIUM_MEMORY_SIGNATURE_SECTION TERBIUM_MEMORY_SIGNATURE_SECTION.*));
        FILL(0);
        . = __SIGNATURE__start + TERBIUM_MEMORY_SIGNATURE_SIZE;
        PROVIDE(__SIGNATURE__end = .);
    }
    PROVIDE(__SIGNATURE__size = SIZEOF(.signature));

    /* Assure that code does not overlap flash data area.*/
    ASSERT((__SIGNATURE__end < TERBIUM_MEMORY_APPLICATION_TOP_ADDRESS), "Error: Code overlaps flash data area.")

    /* Set stack top to end of RAM, and stack limit to the start
     * of stack_dummy section.
     */
    __StackTop   = TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS - 1;
    __StackLimit = __STACK__start;
    PROVIDE(__stack = __StackTop);

    /* Set OpenThread settings flash start and end addresses. */
    __start_ot_flash_data = TERBIUM_MEMORY_ENV_BASE_ADDRESS;
    __stop_ot_flash_data  = TERBIUM_MEMORY_ENV_TOP_ADDRESS;

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT((__StackLimit + SIZEOF(.stack_dummy)) <= __StackTop, "region RAM overflowed with stack")
}
