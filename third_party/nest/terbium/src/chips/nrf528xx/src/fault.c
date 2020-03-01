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

/*
 * @file
 *   This file implements functions for processing fault handlers.
 */

#include <terbium-config.h>

#include <common/logging.hpp>
#include <utils/code_utils.h>
#include <openthread/platform/radio.h>

#include <nrfx/mdk/nrf.h>

#include "platform-nrf5.h"
#include "chips/include/cmsis.h"
#include "chips/include/delay.h"
#include "chips/include/watchdog.h"
#include "platform/backtrace.h"

// Refer http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Cihcfefj.html to get
// these definitions.
#define SCB_CFSR_MMFSR_IACCVIOL ((uint32_t)0x00000001)  ///< Instruction access violation flag.
#define SCB_CFSR_MMFSR_DACCVIOL ((uint32_t)0x00000002)  ///< Data access violation flag.
#define SCB_CFSR_MMFSR_MUNSTKERR ((uint32_t)0x00000008) ///< MemManage fault on unstacking for a return from exception.
#define SCB_CFSR_MMFSR_MSTKERR ((uint32_t)0x00000010)   ///< MemManage fault on stacking for exception entry.
#define SCB_CFSR_MMFSR_MMARVALID ((uint32_t)0x00000080) ///< MemManage Fault Address Register (MMFAR) valid flag.

#define SCB_CFSR_BFSR_IBUSERR ((uint32_t)0x00000100)     ///< Instruction bus error.
#define SCB_CFSR_BFSR_PRECISERR ((uint32_t)0x00000200)   ///< Precise data bus error.
#define SCB_CFSR_BFSR_IMPRECISERR ((uint32_t)0x00000400) ///< Imprecise data bus error.
#define SCB_CFSR_BFSR_UNSTKERR ((uint32_t)0x00000800)    ///< BusFault on unstacking for a return from exception.
#define SCB_CFSR_BFSR_STKERR ((uint32_t)0x00001000)      ///< BusFault on stacking for exception entry.
#define SCB_CFSR_BFSR_BFARVALID ((uint32_t)0x00008000)   ///< BusFault Address Register (BFAR) valid flag.

#define SCB_CFSR_UFSR_UNDEFINSTR ((uint32_t)0x00010000) ///< Undefined instruction UsageFault.
#define SCB_CFSR_UFSR_INVSTATE ((uint32_t)0x00020000)   ///< Invalid state UsageFault.
#define SCB_CFSR_UFSR_INVPC ((uint32_t)0x00040000)      ///< Invalid PC load UsageFault.
#define SCB_CFSR_UFSR_NOCP ((uint32_t)0x00080000)       ///< No coprocessor UsageFault.
#define SCB_CFSR_UFSR_UNALIGNED ((uint32_t)0x01000000)  ///< Unaligned access UsageFault.
#define SCB_CFSR_UFSR_DIVBYZERO ((uint32_t)0x02000000)  ///< Divide by zero UsageFault.

#define FAULT_BACKTRACE_MAX_LEVEL 25    ///< The maximum level of buffer.
#define FAULT_BACKTRACE_MAX_ATTEMPTS 20 ///< The maximum number of attempts to parse buffer.
#define FAULT_BACKTRACE_MIN_LEVEL \
    6 ///< For fault handler, at least 6 levels: tbBacktrace, dumpBacktrace, crashDump, Fault_Handler, main, _start.
#define FAULT_BACKTRACE_MIN_LEVEL_FOR_STACK_OVERFLOW 2 ///< For stack overflow, at least 2 level: main, _start.

#define FAULT_SYSTEM_RESET_DELAY_MS 20 // The delay time before reset device.

/**
 * @struct ExceptionStackFrame
 *
 * This structure represents exception stack frame.
 *
 * Refer http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html to
 * get the exception stack frame layout.
 *
 */
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
    uint32_t stack[0]; // &stack[0] is the original stack address if psr bit[9] is 0.
                       // &stack[1] is the original stack address if psr bit[9] is 1.
} ExceptionStackFrame;

/**
 * Refer: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html
 * Refer: Section B1.5.7 of https://static.docs.arm.com/ddi0403/eb/DDI0403E_B_armv7m_arm.pdf
 *
 * When the processor takes an exception, the processor pushes information onto
 * the current stack. This operation is referred to as stacking and the structure
 * of eight data words is referred as the stack frame. The stack frame contains
 * the following information:
 *
 *            +---------------+
 *            |  <Previous>   |
 *            +---------------+  \
 * (Optional) | Adjusted Word |  |
 *            +---------------+  |
 *            |     xPSR      |  |
 *            +---------------+  |
 *            |      PC       |  |
 *            +---------------+  |
 *            |      LR       |  |
 *            +---------------+  |
 *            |      R12      |  | Exception stack frame
 *            +---------------+  |
 *            |      R3       |  |
 *            +---------------+  |
 *            |      R2       |  |
 *            +---------------+  |
 *            |      R1       |  |
 *            +---------------+  |
 *            |      R0       |  |
 *     SP --> +---------------+  /
 *
 * Immediately after stacking, the stack pointer indicates the lowest address in
 * the stack frame.
 *
 * On an exception entry when CCR.STKALIGN is set to 1, the exception entry sequence
 * ensures that the stack pointer in use before the exception entry has 8-byte
 * alignment, by adjusting its alignment if necessary. When the processor pushes the
 * PSR value to the stack it uses bit[9] of the stacked PSR value to indicate whether
 * it realigned the stack.
 *
 * On an exception return when CCR.STKALIGN is set to 1, the processor uses the value
 * of bit[9] of the PSR value popped from the stack to determine whether it must
 * adjust the stack pointer alignment. This reverses any forced stack alignment
 * performed on the exception entry.
 *
 * The exception stack frame are pushed onto the stack by processor automatically,
 * so the fault handlers won't push LR to the stack. When backtrace searches the
 * LR register, it can't find push/stmd instructions from the fault hander. To
 * handle this case, we simulate the process of CPU pushing the exception stack
 * frame onto the stack.
 * Here are the detailed steps:
 * <1> If the stack was not realigned, push two registers onto the stack. When
 *     backtrace find this instruction, it will pop PC and xPSR.
 *     If the stack was realigned, push three registers onto the stack. When
 *     backtrace find this instruction, it will pop PC, xPSR and adjusted word.
 * <2> Pop the two or three registers. This ensures that the stack pointer
 *     points to the exception stack frame.
 * <3> Push the LR onto the stack. When backtrace find this instruction, it will
 *     pop LR and get the LR.
 * <4> Pop the LR. This ensures that the stack pointer points to the exception
 *     stack frame.
 * <5> Subtract 20 from SP. When backtrace find this instruction, it will pop
 *     R0, R1, R2 ,R3 and R12.
 * <6> Add 20 to SP. This ensures that the stack pointer points to the
 *     exception stack frame.
 */

/**
 * This macro simulate the process of CPU pushing the exception stack frame onto
 * the stack and then calls the function `crashDump()`.
 *
 * @param[in] aFault  Fault type.
 *
 * @param[out] r0  Exception stack frame address.
 * @param[out] r1  Fault Type.
 *
 */
#define CALL_FALUT_HANDLER(aFault)                                                                            \
    __asm volatile(                                                                                           \
        " bl  crashDisableMpu            \n" /* Disable MPU. In case the fault is caused by stack overflow.*/ \
        " mrs r0, msp                    \n" /* Read MSP to R0. */                                            \
        " mov r1, #" aFault "            \n" /* Set R1 to fault type. */                                      \
        " ldr r2, [sp, #28]              \n" /* Save the xPSR to R2. */                                       \
        " tst r2, #512                   \n" /* Check if the bit[9] of xPSR is set to 1. */                   \
        " bne StackRealigned" aFault "   \n" /* The bit 9 is 1, the exception stack was realigned. */         \
        " beq StackNotRealigned" aFault "\n" /* The bit 9 is 0, the exception stack was not realigned. */     \
                                                                                                              \
        " StackRealigned" aFault ":      \n"                                                                  \
        " push {r0, r1, r2}              \n" /* Used by backtrace to skip PC, xPSR and adjusted word. */      \
        " pop {r0, r1, r2}               \n"                                                                  \
        " push {lr}                      \n" /* Used by backtrace to skip and get the real return address. */ \
        " pop {lr}                       \n"                                                                  \
        " sub sp, #20                    \n" /* Used by backtrace to skip R0, R1, R2 ,R3 and R12. */          \
        " add sp, #20                    \n"                                                                  \
        " bl crashDump                   \n" /* Call crashDump(). */                                          \
        " b .                            \n" /* Dead loop. */                                                 \
                                                                                                              \
        " StackNotRealigned" aFault ":   \n"                                                                  \
        " push {r0, r1}                  \n" /* Used by backtrace to skip PC and xPSR. */                     \
        " pop {r0, r1}                   \n"                                                                  \
        " push {lr}                      \n" /* Used by backtrace to skip and get the real return address. */ \
        " pop {lr}                       \n"                                                                  \
        " sub sp, #20                    \n" /* Used by backtrace to skip R0, R1, R2, R3 and R12. */          \
        " add sp, #20                    \n"                                                                  \
        " bl crashDump                   \n"  /* Call crashDump(). */                                         \
        " b .                            \n") /* Dead loop. */

static void __attribute__((used)) crashDisableMpu(void)
{
    // If the MPU is used to guard stack, it could prevent an exception
    // frame from being pushed onto the stack. It also prevent other functions
    // from pushing registers or data onto the stack. This function
    // disables the MPU to allow processor to process fault handler.

#if (__MPU_PRESENT == 1) /* Same check used in CMSIS header */
    MPU->CTRL = 0;
    __DSB();
    __ISB();
#endif
}

static void dumpMemFaultCode(uint32_t aPc)
{
    if ((SCB->CFSR & SCB_CFSR_MMFSR_IACCVIOL) != 0)
    {
        otLogCritPlat("%s: Instruction Access Violation Fault, PC=0x%08lx", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_MMFSR_DACCVIOL) != 0)
    {
        uint32_t mmfar = 0;

        if ((SCB->CFSR & SCB_CFSR_MMFSR_MMARVALID) != 0)
        {
            mmfar = SCB->MMFAR;
        }

        otLogCritPlat("%s: Data Access Violation Fault, PC=0x%08lx SCB->MMFAR=0x%08lx", __func__, aPc, mmfar);
    }
    else if ((SCB->CFSR & SCB_CFSR_MMFSR_MUNSTKERR) != 0)
    {
        otLogCritPlat("%s: Memory Unstacking Fault", __func__);
    }
    else if ((SCB->CFSR & SCB_CFSR_MMFSR_MSTKERR) != 0)
    {
        otLogCritPlat("%s: Memory Stacking Fault", __func__);
    }
    else
    {
        otLogCritPlat("%s: Unknown MemFault", __func__);
    }
}

static void dumpBusFaultCode(uint32_t aPc)
{
    if ((SCB->CFSR & SCB_CFSR_BFSR_IBUSERR) != 0)
    {
        otLogCritPlat("%s: Instruction Bus Fault", __func__);
    }
    else if ((SCB->CFSR & SCB_CFSR_BFSR_PRECISERR) != 0)
    {
        uint32_t bfar = 0;

        if ((SCB->CFSR & SCB_CFSR_BFSR_BFARVALID) != 0)
        {
            bfar = SCB->BFAR;
        }

        otLogCritPlat("%s: Precise Data Bus Error Fault, PC=0x%08lx SCB->BFAR=0x%08lx", __func__, aPc, bfar);
    }
    else if ((SCB->CFSR & SCB_CFSR_BFSR_IMPRECISERR) != 0)
    {
        otLogCritPlat("%s: Imprecise Data Bus Error Fault", __func__);
    }
    else if ((SCB->CFSR & SCB_CFSR_BFSR_UNSTKERR) != 0)
    {
        otLogCritPlat("%s: Bus Unstacking Fault", __func__);
    }
    else if ((SCB->CFSR & SCB_CFSR_BFSR_STKERR) != 0)
    {
        otLogCritPlat("%s: Bus Stacking Fault", __func__);
    }
    else
    {
        otLogCritPlat("%s: Unknown BusFault", __func__);
    }
}

static void dumpUsageFaultCode(uint32_t aPc)
{
    if ((SCB->CFSR & SCB_CFSR_UFSR_UNDEFINSTR) != 0)
    {
        otLogCritPlat("%s: Undefined Instruction Usage Fault, PC=0x%08lx", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_UFSR_INVSTATE) != 0)
    {
        otLogCritPlat("%s: Invalid State Fault, PC=0x%08lx", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_UFSR_INVPC) != 0)
    {
        otLogCritPlat("%s: Invalid Program Counter Fault, PC=0x%08lx", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_UFSR_NOCP) != 0)
    {
        otLogCritPlat("%s: No Coprocessor Fault, PC=0x%08lx", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_UFSR_UNALIGNED) != 0)
    {
        otLogCritPlat("%s: Unaligned Access Fault, PC=0x%08x", __func__, aPc);
    }
    else if ((SCB->CFSR & SCB_CFSR_UFSR_DIVBYZERO) != 0)
    {
        otLogCritPlat("%s: Divide By Zero Fault, PC=0x%08lx", __func__, aPc);
    }
    else
    {
        otLogCritPlat("%s: Unknown UsageFault, PC=0x%08lx", __func__, aPc);
    }
}

static void dumpFaultCode(uint32_t aPc)
{
    if ((SCB->HFSR & SCB_HFSR_VECTTBL_Msk) != 0)
    {
        // Indicates a BusFault on a vector table read during exception processing.
        otLogCritPlat("%s: Vector Table Hard Fault", __func__);
    }
    else if (SCB->SHCSR & SCB_SHCSR_MEMFAULTACT_Msk)
    {
        // MemManage exception is active.
        dumpMemFaultCode(aPc);
    }
    else if (SCB->SHCSR & SCB_SHCSR_BUSFAULTACT_Msk)
    {
        // BusFault exception is active.
        dumpBusFaultCode(aPc);
    }
    else if (SCB->SHCSR & SCB_SHCSR_USGFAULTACT_Msk)
    {
        // UsageFault exception is active.
        dumpUsageFaultCode(aPc);
    }
    else
    {
        otLogCritPlat("%s:Unknown Hard Fault: SHCSR=0x%08lx CCR=0x%08lx HFSR=0x%08lx CFSR=0x%08lx MMFAR=0x%08lx "
                      "BFAR=0x%08lx DFSR=0x%08lx",
                      __func__, SCB->SHCSR, SCB->CCR, SCB->HFSR, SCB->CFSR, SCB->MMFAR, SCB->BFAR, SCB->DFSR);
    }
}

#if TERBIUM_CONFIG_BACKTRACE_ENABLE
__attribute__((always_inline)) static inline uint32_t getSp(void)
{
    register uint32_t result;
    __asm volatile("mov %0, sp\n" : "=r"(result));
    return (result);
}

static void dumpBacktrace(void)
{
    extern uint32_t __StackTop;
    uint32_t        buffer[FAULT_BACKTRACE_MAX_LEVEL];
    uint8_t         level;
    uint8_t         i;
    char            string[250];
    char *          start = string;
    char *          end   = string + sizeof(string);
    uint32_t        sp    = getSp();

    level = tbBacktrace(buffer, otARRAY_LENGTH(buffer));

    // If the tbBacktrace() returned short backtrace (possibly due to MPU preventing
    // an exception frame from being pushed onto the faulting stack), then use
    // tbBacktraceNoContext() to get a longer list of possible LR values from the
    // stack because we presume that the former failed for some reason (bad stack frame).
    if (level < FAULT_BACKTRACE_MIN_LEVEL)
    {
        start += snprintf(start, end - start, "(NoContext) ");
        level = tbBacktraceNoContext(sp, (uint32_t)&__StackTop, buffer, otARRAY_LENGTH(buffer),
                                     FAULT_BACKTRACE_MIN_LEVEL_FOR_STACK_OVERFLOW, FAULT_BACKTRACE_MAX_ATTEMPTS);
    }

    for (i = 0; i < level; i++)
    {
        start += snprintf(start, end - start, "0x%lx ", buffer[i]);
    }

    // Output the image version, so that we can find the map file to decode the buffer.
    otLogCritPlat("%s: %s", __func__, otPlatRadioGetVersionString(NULL));
    otLogCritPlat("%s: %s", __func__, string);
}
#endif // TERBIUM_CONFIG_BACKTRACE_ENABLE

static void __attribute__((used)) crashDump(uint32_t *aExceptionStackAddress, uint32_t aType)
{
    ExceptionStackFrame *exceptionStackFrame = (ExceptionStackFrame *)aExceptionStackAddress;

    OT_UNUSED_VARIABLE(aType);

    // Refresh watchdog.
    tbHalWatchdogRefresh();

    // Dump fault code.
    dumpFaultCode(exceptionStackFrame->pc);

#if TERBIUM_CONFIG_BACKTRACE_ENABLE
    // Dump backtrace of current context.
    dumpBacktrace();
#endif

    otLogCritPlat("%s: Resetting ...", __func__);

    // Wait for the log to be output.
    tbHalDelayMs(FAULT_SYSTEM_RESET_DELAY_MS);

    // Reset the device.
    NVIC_SystemReset();
}

void nrf5FaultInit(void)
{
    // Enable Usage Fault, Bus Fault and Memory Management Fault.
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;

    // Enable divide by 0 usage faults.
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

void nrf5FaultDeinit(void)
{
    // Disable Usage Fault, Bus Fault and Memory Management Fault.
    SCB->SHCSR &= (~SCB_SHCSR_USGFAULTENA_Msk) & (~SCB_SHCSR_BUSFAULTENA_Msk) & (~SCB_SHCSR_MEMFAULTENA_Msk);

    // Disable divide by 0 usage faults.
    SCB->CCR &= ~SCB_CCR_DIV_0_TRP_Msk;
}

void HardFault_Handler(void)
{
    // Hard Fault: is caused by Bus Fault, Memory Management Fault, or Usage Fault if
    // their handler cannot be executed.

    CALL_FALUT_HANDLER("0");
}

void MemoryManagement_Handler(void)
{
    // Memory Management Fault: detects memory access violations to regions that are
    // defined in the Memory Management Unit (MPU); for example code execution from
    // a memory region with read/write access only.

    CALL_FALUT_HANDLER("1");
}

void BusFault_Handler(void)
{
    // Bus Fault: Detects memory access errors on instruction fetch, data read/write,
    // interrupt vector fetch, and register stacking (save/restore) on interrupt (entry/ exit).

    CALL_FALUT_HANDLER("2");
}

void UsageFault_Handler(void)
{
    // Usage Fault: detects execution of undefined instructions, unaligned memory
    // access for load/store multiple. When enabled, divide-by-zero and other unaligned
    // memory accesses are also detected.

    CALL_FALUT_HANDLER("3");
}
