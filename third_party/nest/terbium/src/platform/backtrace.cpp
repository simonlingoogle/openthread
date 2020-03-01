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
 *   This file implements functions for ARM backtrace.
 */

/**
 * ARM®v7-M Architecture Reference Manual:
 *     - https://static.docs.arm.com/ddi0403/eb/DDI0403E_B_armv7m_arm.pdf
 *
 * Theory of operation:
 *
 * There are two types of functions:
 *     - non-leaf: functions that make a call to another function
 *     - leaf: functions that don't call other function.
 *
 * For non-leaf functions, the compiler always follows the pattern below when
 * generating the code:
 *
 * The prologue of the function looks like this: a push (or store multiple and
 * decrement with SP as destination), and an optional SP decrement for local
 * variables:
 *     push/stmd  sp!, {rx, ry, ....., lr}
 *     sub sp, sp, #xx
 *
 * The epilogue has to undo what the prologue did in order to leave the SP
 * where it was before the function call:
 *     add sp, sp, #xx
 *     pop/ldm   sp!, {rx, ry, ....., pc}
 *
 * The backtrace function takes two parameters: aPc, aSp.
 * The logic to follow is:
 *     - make currentPc=aPc, currentSp=aSp.
 *     - walk backward from the currentPc until we find either a push or a
 *       stmfd with SP as destination.
 *     - decode the push/stmfd instruction to find out how many registers got pushed.
 *     - look for the "sub sp" instruction and decode it (it's usually the
 *       first or second instruction down from the push.
 *     - check if there was an additional "sub sp" before the push (this
 *       appears to be used in conjunction with variable arguments)
 *     - compute currentSp = SP + 4 * num_of_registers_pushed + space that
 *       was subtracted for the local variables.
 *     - compute LR = currentSp[-1]
 *     - lather, rinse and repeat
 *
 * This procedure works fairly well for non-leaf function but doesn't work for
 * leafs. So if we get a crasher in a leaf we don't get the right backtrace.
 * Unfortunately leafs are just as likely to crash.
 *
 * The most important leaf function for us is the fault handler. Generally,
 * when an exception occurs, CPU automatically pushes LR onto stack. Then
 * the fault handler does not contain instructions to push LR onto the stack.
 * which causes the backtrace can't find the correct PUSH LR instruction based
 * on PC. The fault handler can simulate the process of CPU pushing exception
 * stack frame onto the stack to handler this case.
 */

#include <terbium-config.h>

#include <common/code_utils.hpp>
#include <common/logging.hpp>

#include "platform/backtrace.h"

#define INVALID_PC 0xffffffff ///< Invalid PC value.

#if TERBIUM_CONFIG_BACKTRACE_ENABLE
static uint32_t countBitSet(uint32_t aValue)
{
    // K&R style count bit set.
    uint32_t i;

    for (i = 0; aValue; i++)
    {
        aValue &= (aValue - 1);
    }

    return i;
}

static bool isAddressValidForCode(uint32_t aAddress)
{
    extern uint32_t __TEXT__start; // Defined in linker script.
    extern uint32_t __TEXT__end;   // Defined in linker script.

    return (aAddress > ((uint32_t)&__TEXT__start) && aAddress <= ((uint32_t)&__TEXT__end));
}

static bool isAddressValidForStack(uint32_t aAddress)
{
    // The stack may overflow, so we are using RAM range to check the validity of the address.
    return (aAddress > TERBIUM_MEMORY_SOC_DATA_RAM_BASE_ADDRESS) &&
           (aAddress <= TERBIUM_MEMORY_SOC_DATA_RAM_TOP_ADDRESS);
}

static uint32_t getModifiedImmediateConstant(uint32_t aInstruction)
{
    uint8_t  i_imm3_a;
    uint32_t immediateConstant = 0;

    // See A5.3.2 "Modified immediate constants in Thumb instructions" in the
    // ARMv7-M Architectural Reference Manual, which explains how constants
    // are encoded.
    i_imm3_a =
        ((aInstruction & (0x1 << 26)) >> 22) | ((aInstruction & (0x7 << 12)) >> 11) | ((aInstruction & 0x80) >> 7);

    if (i_imm3_a < 0x8)
    {
        uint32_t imm8 = aInstruction & 0xff;

        // When i:imm3:a is less than 8, the lower two bits of imm3 give the
        // following special encodings.
        switch (i_imm3_a >> 1)
        {
        case 0x0:
            immediateConstant = imm8;
            break;

        case 0x1:
            immediateConstant = imm8 << 16 | imm8;
            break;

        case 0x2:
            immediateConstant = imm8 << 24 | imm8 << 8;
            break;

        case 0x3:
            immediateConstant = imm8 << 24 | imm8 << 16 | imm8 << 8 | imm8;
            break;
        }
    }
    else
    {
        // When i:imm3:a is greater than or equal to 8, it encodes a shift of
        // imm8. Since i:imm3:a is 5-bits, its range here will be between 8 and
        // 31 inclusive.
        immediateConstant = ((aInstruction & 0x7f) | 0x80) << (32 - i_imm3_a);
    }

    return immediateConstant;
}

/**
 * Unwind the stack frame indicated by pc and sp.
 *
 * The return value is INVALID_PC if no lr-push could be found. Otherwise it is
 * the found lr.
 *
 * sp is modified according to any stack pointer adjustments found in the
 * current frame.
 *
 * The functions iterates backwards from pc, instruction by instruction, in two
 * phases:
 *
 * 1. "lr search":
 *    Searching backwards from a (likely) faulting pc, looking for the
 *    instruction which pushed the link register. Along the way, any stack
 *    pointer adjustments from sub-sp instructions should be counted. This
 *    search continues indefinitely until either the push is found or an
 *    invalid pc is encountered.
 *
 * 2. "prologue search":
 *   If the lr search phase is successful, the search for sp modifying
 *   instructions continues for PROLOGUE_SEARCH_LEN instructions. This search
 *   includes both sub-sp instructions and non-lr pushing instructions.
 */
static uint32_t findLrAndSpForFrame(uint32_t aPc, uint32_t *aSp)
{
// Note: When the field W == '1' && Rn == '1101' in instruction STMDB,
//       the instruction STMDB equals to instruction PUSH Encoding T2.
#define ARM_PUSH_T1_MASK 0xFF00       ///< A7.7.99, Encoding T1.
#define ARM_PUSH_W_LR_T1_COMP 0xB500  ///< A7.7.99, Encoding T1.
#define ARM_PUSH_NO_LR_T1_COMP 0xB400 ///< A7.7.99, Encoding T1.
#define ARM_PUSH_T1_REG_MASK 0x01FF   ///< A7.7.99, Encoding T1.

#define ARM_PUSH_T2_MASK 0xFFFFE000       ///< A7.7.99, Encoding T2.
#define ARM_PUSH_W_LR_T2_COMP 0xE92D4000  ///< A7.7.99, Encoding T2.
#define ARM_PUSH_NO_LR_T2_COMP 0xE92D0000 ///< A7.7.99, Encoding T2.
#define ARM_PUSH_T2_REG_MASK 0x0000FFFF   ///< A7.7.99, Encoding T2.

#define ARM_SUB_SP_IMM_T1_MASK 0xFF80     ///< A7.7.173, Encoding T1.
#define ARM_SUB_SP_IMM_T1_COMP 0xB080     ///< A7.7.173, Encoding T1.
#define ARM_SUB_SP_IMM_T2_MASK 0xFBEF8F00 ///< A7.7.173, Encoding T2.
#define ARM_SUB_SP_IMM_T2_COMP 0xF1AD0D00 ///< A7.7.173, Encoding T2.
#define ARM_SUB_SP_IMM_T3_MASK 0xFBFF8F00 ///< A7.7.173, Encoding T3.
#define ARM_SUB_SP_IMM_T3_COMP 0xF2AD0D00 ///< A7.7.173, Encoding T3.

#define PROLOGUE_SEARCH_LEN 2 ///< Prologue search length.

    typedef enum
    {
        kSearchLr,       ///< Search PUSH LR instruction.
        kSearchLrDone,   ///< Found PUSH LR instruction.
        kSearchPrologue, ///< Search prologue.
    } SearchPhase;

    uint32_t returnLr = INVALID_PC;
    uint32_t spOffset = 0; // spOffset is used to accumulate stack pointer modifications in register
                           // (4-byte) increments.

    // Only match lr-pushes during lr search.
    uint32_t push32Comp = ARM_PUSH_W_LR_T2_COMP;
    uint16_t push16Comp = ARM_PUSH_W_LR_T1_COMP;

    SearchPhase searchPhase       = kSearchLr;
    uint16_t    searchPrologueLen = PROLOGUE_SEARCH_LEN;
    uint16_t    instruction16     = 0;
    uint32_t    pc                = aPc;

    while (1)
    {
        uint16_t lastInstruction16;
        uint32_t instruction32;

        if (!isAddressValidForCode(pc))
        {
            break;
        }

        lastInstruction16 = instruction16;
        instruction16     = *((uint16_t *)pc);
        instruction32     = (instruction16 << 16) | lastInstruction16;

        // Look for relevant instructions. There are five total, listed here
        // with the referenced section from the ARMv7-M Architecture Reference
        // Manual.
        // 1. 16-bit push       PUSH<c> <registers>             A7.7.99  (Encoding T1)
        // 2. 32-bit push       PUSH<c>.W <registers>           A7.7.99  (Encoding T2)
        // 3. 16-bit sub sp     SUB<c> SP,SP,#<imm7>            A7.7.173 (Encoding T1)
        // 4. 32-bit sub sp     SUB{S}<c>.W <Rd>,SP,#<const>    A7.7.173 (Encoding T2)
        // 5. 32-bit sub sp     SUBW<c> <Rd>,SP,#<imm12>        A7.7.173 (Encoding T3)
        if ((instruction32 & ARM_PUSH_T2_MASK) == push32Comp)
        {
            uint32_t regsPushed;

            regsPushed = countBitSet(instruction32 & ARM_PUSH_T2_REG_MASK);
            spOffset += regsPushed;

            if (searchPhase == kSearchLr)
            {
                searchPhase = kSearchLrDone;
            }
        }
        else if ((instruction32 & ARM_SUB_SP_IMM_T2_MASK) == ARM_SUB_SP_IMM_T2_COMP)
        {
            uint32_t imm32;

            // Encoding T2 of SUB (SP minus immediate) encodes the immediate
            // constant as a 'modified immediate constant'.
            imm32 = getModifiedImmediateConstant(instruction32);
            spOffset += imm32 / 4;
        }
        else if ((instruction32 & ARM_SUB_SP_IMM_T3_MASK) == ARM_SUB_SP_IMM_T3_COMP)
        {
            uint32_t imm32;

            // Encoding T3 of SUB (SP minus immediate) encodes the immediate
            // constant in three disjoint bitfields, i:imm3:imm8.
            imm32 =
                ((instruction32 & (0x1 << 26)) >> 15) | ((instruction32 & (0x7 << 12)) >> 4) | (instruction32 & 0xff);
            spOffset += imm32 / 4;
        }
        else if ((instruction16 & ARM_PUSH_T1_MASK) == push16Comp)
        {
            uint32_t regsPushed;

            regsPushed = countBitSet(instruction16 & ARM_PUSH_T1_REG_MASK);
            spOffset += regsPushed;

            if (searchPhase == kSearchLr)
            {
                searchPhase = kSearchLrDone;
            }
        }
        else if ((instruction16 & ARM_SUB_SP_IMM_T1_MASK) == ARM_SUB_SP_IMM_T1_COMP)
        {
            uint32_t imm32;

            // Encoding T1 of SUB (SP minus immediate) encodes the immediate
            // constant in the lower 7 bits (imm7). It is interpreted as imm7:'00'
            // so that it doesn't need to be divided by 4 before being added to
            // sp_change.
            imm32 = instruction16 & 0x7f;
            spOffset += imm32;
        }

        if (searchPhase == kSearchLrDone)
        {
            // Found the lr push. Check the modified sp before dereferencing.
            uint32_t  tmpLr;
            uint32_t *tmpSp = (uint32_t *)(*aSp) + spOffset;

            if (!isAddressValidForStack((uint32_t)tmpSp))
            {
                break;
            }

            // lr should be last register pushed, which on a descending stack
            // means that it is at -1 offset.
            tmpLr = tmpSp[-1];

            if (!isAddressValidForCode(tmpLr))
            {
                break;
            }

            returnLr = tmpLr;

            // Switch to prologue search.
            searchPhase = kSearchPrologue;
            push32Comp  = ARM_PUSH_NO_LR_T2_COMP;
            push16Comp  = ARM_PUSH_NO_LR_T1_COMP;
        }

        if ((searchPhase == kSearchPrologue) && (searchPrologueLen-- == 0))
        {
            break;
        }

        // Decrement the pc by one 16-bit instruction.
        pc -= 2;
    }

    *aSp += spOffset * 4;

    return returnLr;
}

static uint16_t backtrace(uint32_t aPc, uint32_t aSp, uint32_t *aBuffer, uint16_t aLength)
{
    uint16_t level     = 0;
    uint32_t currentSp = aSp;
    uint32_t currentPc = aPc;

    VerifyOrExit((aBuffer != NULL) && (aLength != 0), OT_NOOP);
    VerifyOrExit(isAddressValidForCode(aPc), OT_NOOP);

    while (level < aLength)
    {
        // Save the PC.
        *aBuffer++ = currentPc;
        level++;

        // Strip the Thumb bit before passing to `findLrAndSpForFrame()`.
        currentPc = currentPc & ~0x1;

        currentPc = findLrAndSpForFrame(currentPc, &currentSp);

        if (currentPc == INVALID_PC)
        {
            break;
        }
    }

exit:
    return level;
}

uint16_t tbBacktraceNoContext(uint32_t  aStackPointer,
                              uint32_t  aStackTop,
                              uint32_t *aBuffer,
                              uint16_t  aLength,
                              uint16_t  aMinLevel,
                              uint16_t  aMaxAttempts)
{
    uint32_t *currentSp  = (uint32_t *)aStackPointer;
    uint16_t  attempts   = 0;
    uint16_t  level      = 0;
    uint16_t  maxLevel   = 0;
    uint32_t  maxLevelLr = 0;
    uint32_t *maxLevelSp = NULL;

    // This method finds at most aMaxAttempts possible LRs from stack and then
    // call `backtrace()` to parse backtraces. The maximum level of the backtraces
    // will be recorded to compare with aMinLevel. If the maxLevel is smaller
    // than the aMinLevel size, we then fall back to just returning an array of
    // possible LR values scanning from the aStackPointer to aStackTop, stopping at
    // aLength entries or when we hit an invalid SP, whichever is hit first.

    while (currentSp < (uint32_t *)aStackTop)
    {
        uint32_t possibleLr = *currentSp++;

        if ((possibleLr & 0x1) && isAddressValidForCode(possibleLr))
        {
            level = backtrace(possibleLr, (uint32_t)currentSp, aBuffer, aLength);

            // Find backtraces with maxmimum depth.
            if (level > maxLevel)
            {
                maxLevel   = level;
                maxLevelLr = possibleLr;
                maxLevelSp = currentSp;
            }

            if (attempts++ >= aMaxAttempts)
            {
                break;
            }
        }
    }

    if (maxLevel >= aMinLevel)
    {
        // Return the backtraces with maxmimum depth.
        level = backtrace(maxLevelLr, (uint32_t)maxLevelSp, aBuffer, aLength);
    }
    else
    {
        // Backtrace unable to find a valid backtrace, just return guesses of possible LRs.
        currentSp = (uint32_t *)aStackPointer;
        level     = 0;

        while (level < aLength && (currentSp < (uint32_t *)aStackTop))
        {
            uint32_t possibleLr = *currentSp++;

            if ((possibleLr & 0x1) && isAddressValidForCode(possibleLr))
            {
                aBuffer[level++] = possibleLr;
            }
        }
    }

    return level;
}

static inline uint32_t getSp(void)
{
    register uint32_t result;
    __asm volatile("mov %0, sp\n" : "=r"(result));
    return (result);
}

static inline uint32_t getPc(void)
{
    register uint32_t result;
    __asm volatile("mov %0, pc\n" : "=r"(result));
    return (result);
}

uint16_t tbBacktrace(uint32_t *aBuffer, uint16_t aLength)
{
    uint16_t level     = 0;
    uint32_t currentPc = getPc();
    uint32_t currentSp = getSp();

    // This function doesn't directly call `backtrace()`. Because if the function `tbBacktrace()`
    // directly call `backtrace()`, the compiler optimizes the function `tbBacktrace()`, which
    // does not push LR to the stack. Then the backtrace can't find the correct PUSH instruction.

    VerifyOrExit((aBuffer != NULL) && (aLength != 0), OT_NOOP);

    while (level < aLength)
    {
        // Save the PC.
        *aBuffer++ = currentPc;
        level++;

        // Strip the Thumb bit before passing to `findLrAndSpForFrame()`.
        currentPc = currentPc & ~0x1;

        currentPc = findLrAndSpForFrame(currentPc, &currentSp);

        if (currentPc == INVALID_PC)
        {
            break;
        }
    }

exit:
    return level;
}

#endif // TERBIUM_CONFIG_BACKTRACE_ENABLE
