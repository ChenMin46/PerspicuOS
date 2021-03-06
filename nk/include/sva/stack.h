//===-- stack.h -----------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Stack-switching and other secure entry point operations.
//
//===----------------------------------------------------------------------===//
//
// Defines macro that can be used to define functions that act as
// entry points into the secure kernel.
//
// Sample use:
//
// SECURE_WRAPPER(int, TestFunc, int a, int b) {
//  return a + b;
//}
//
// This will define a function 'TestFunc' that acts as a wrapper
// for 'TestFunc_secure' which will contain the usual function code.
//
// The wrapper function handles operations needed to enter secure
// execution (see SECURE_ENTRY macro for high-level description)
// and return to normal execution (see SECURE_EXIT).
//
// As implemented, these are not reentrant.
//
//===----------------------------------------------------------------------===//
//
// TODO:
//  * Use per-CPU stacks for SMP operation
//
//===----------------------------------------------------------------------===//

#ifndef _STACK_SWITCH_
#define _STACK_SWITCH_

#include <stdint.h>

//===-- Secure Stack Switching --------------------------------------------===//

// Points to top of secure stack
// XXX: Whoever defines this should ensure the region is write-protected!
extern const uintptr_t SecureStackBase;

// TODO: Manage stack per-cpu, do lookup here
// Use only RAX/RCX registers to accomplish this.
// (Or spill more in calling context)

#define SWITCH_TO_SECURE_STACK                                                 \
  /* Spill registers for temporary use */                                      \
  "movq %rax, -8(%rsp)\n"                                                      \
  "movq %rcx, -16(%rsp)\n"                                                     \
  "movq SecureStackBase, %rax\n"                                               \
  /* Save normal stack pointer in rcx and on secure stack */                   \
  "mov %rsp, %rcx\n"                                                           \
  "mov %rsp, -8(%rax)\n"                                                       \
  "subq $8, %rax\n"                                                            \
  /* Switch to secure stack! */                                                \
  "movq %rax, %rsp\n"                                                          \
  /* Restore spilled registers from original stack (rcx) */                    \
  "movq -8(%rcx), %rax\n"                                                      \
  "movq -16(%rcx), %rcx\n"                                                     \
      /* Carry on, my wayward kernel. */                                       \
      /* There'll be peace when you are done... */

#define SWITCH_BACK_TO_NORMAL_STACK                                            \
  /* Top of secure stack contains original stack pointer, restore it! */       \
  /* First, save rax/rcx for temporary use */                                  \
  "movq %rax, -8(%rsp)\n"                                                      \
  "movq %rcx, -16(%rsp)\n"                                                     \
  /* Save secure stack pointer for restoring these spilled registers */        \
  "movq %rsp, %rcx\n"                                                          \
  /* Grab original stack pointer and switch to it */                           \
  "movq 0(%rsp), %rsp\n"                                                       \
  /* Restore spilled registers */                                              \
  "movq -8(%rcx), %rax\n"                                                      \
  "movq -16(%rcx), %rcx\n"

//===-- Interrupt Flag Control --------------------------------------------===//

#define DISABLE_INTERRUPTS                                                     \
  /* Save current flags */                                                     \
  "pushf\n"                                                                    \
  /* Disable interrupts */                                                     \
  "cli\n"

#define ENABLE_INTERRUPTS                                                      \
  /* Restore flags, enabling interrupts if they were before */                 \
  "popf\n"

//===-- Write-Protect Control ---------------------------------------------===//

// TODO: Check calling convention for free register(s)
#define DISABLE_WP_BIT                                                         \
  /* Save scratch register to stack */                                         \
  "pushq %rax\n"                                                               \
  /* Get current cr0 value */                                                  \
  "movq %cr0, %rax\n"                                                          \
  /* Clear WP bit in copy */                                                   \
  "andq $0xfffffffffffeffff, %rax\n"                                           \
  /* Replace cr0 with updated value */                                         \
  "movq %rax, %cr0\n"                                                          \
  /* Restore clobbered register */                                             \
  "popq %rax\n"

#define ENABLE_WP_BIT                                                          \
  /* Save scratch register to stack */                                         \
  "pushq %rax\n"                                                               \
  /* Get current cr0 value */                                                  \
  "movq %cr0, %rax\n"                                                          \
  /* Set WP bit in copy */                                                     \
  "orq $0x10000, %rax\n"                                                       \
  /* Replace cr0 with updated value */                                         \
  "movq %rax, %cr0\n"                                                          \
  /* Restore clobbered register */                                             \
  "popq %rax\n"

//===-- Entry/Exit High-Level Descriptions --------------------------------===//

#ifdef __MODULAR_AND_READABLE
#define SECURE_ENTRY                                                           \
  DISABLE_INTERRUPTS                                                           \
  DISABLE_WP_BIT                                                               \
  SWITCH_TO_SECURE_STACK

#define SECURE_EXIT                                                            \
  SWITCH_BACK_TO_NORMAL_STACK                                                  \
  ENABLE_WP_BIT                                                                \
  ENABLE_INTERRUPTS
#else
// More optimized variants

#define SECURE_ENTRY                                                           \
  /* Save current flags */                                                     \
  "pushf\n"                                                                    \
  /* Disable interrupts */                                                     \
  "cli\n"                                                                      \
  /* Spill registers for temporary use */                                      \
  "movq %rax, -8(%rsp)\n"                                                      \
  "movq %rcx, -16(%rsp)\n"                                                     \
  /* Save initial stack pointer in rcx */                                      \
  "movq %rsp, %rcx\n"                                                          \
  /* Get current cr0 value */                                                  \
  "movq %cr0, %rax\n"                                                          \
  /* Clear WP bit in copy */                                                   \
  "andq $0xfffffffffffeffff, %rax\n"                                           \
  /* Replace cr0 with updated value */                                         \
  "movq %rax, %cr0\n"                                                          \
  /* Disable interrupts */                                                     \
  "cli\n"                                                                      \
  /* Switch to secure stack! */                                                \
  "movq SecureStackBase, %rsp\n"                                               \
  /* Save original stack pointer for later restoration */                      \
  "pushq %rcx\n"                                                               \
  /* Restore spilled registers from original stack (rcx) */                    \
  "movq -8(%rcx), %rax\n"                                                      \
  "movq -16(%rcx), %rcx\n"

#define SECURE_EXIT                                                            \
  /* Switch back to original stack */                                          \
  "movq 0(%rsp), %rsp\n"                                                       \
  /* Save scratch register to stack */                                         \
  "pushq %rax\n"                                                               \
  /* Get current cr0 value */                                                  \
  "movq %cr0, %rax\n"                                                          \
  "1:\n"                                                                       \
  /* Set bit for WP enable */                                                  \
  "orq $0x10000, %rax\n"                                                       \
  /* Replace cr0 with updated value */                                         \
  "movq %rax, %cr0\n"                                                          \
  /* Ensure WP bit was set */                                                  \
  "test $0x10000, %eax\n"                                                      \
  "je 1b\n"                                                                    \
  /* Restore clobbered register */                                             \
  "popq %rax\n"                                                                \
  /* Restore flags, enabling interrupts if they were before */                 \
  "popf\n"

#endif

//===-- Wrapper macro for marking Secure Entrypoints ----------------------===//

#define SECURE_WRAPPER(RET, FUNC, ...) \
asm( \
  ".text\n" \
  ".globl " #FUNC "\n" \
  ".align 16,0x90\n" \
  ".type " #FUNC ",@function\n" \
  #FUNC ":\n" \
  ".cfi_startproc\n" \
  /* Do whatever's needed on entry to secure area */ \
  SECURE_ENTRY \
  /* Call real version of function */ \
  "call " #FUNC "_secure\n" \
  /* Operation complete, go back to unsecure mode */ \
  SECURE_EXIT \
  "ret\n" \
  #FUNC "_end:\n" \
  ".size " #FUNC ", " #FUNC "_end - " #FUNC "\n" \
  ".cfi_endproc\n" \
); \
RET FUNC ##_secure(__VA_ARGS__); \
RET __attribute__((visibility("hidden"))) FUNC ##_secure(__VA_ARGS__)

//===-- Wrapper macro for calling secure functions from secure context ---===//

#define SECURE_CALL(FUNC, ...) \
{ \
    extern typeof(FUNC) FUNC ##_secure; \
    (void)FUNC ##_secure(__VA_ARGS__); \
}

//===-- Utilities for accessing original context from secure functions ----===//

static inline uintptr_t get_insecure_context_rsp() {
  // Original RSP is first thing put on the secure stack:
  uintptr_t *ptr = (uintptr_t *)SecureStackBase;
  return ptr[-1];
}

static inline uintptr_t get_insecure_context_flags() {
  // Insecure flags are stored on the insecure stack:
  uintptr_t *ptr = (uintptr_t *)get_insecure_context_rsp();
  return ptr[0];
}

static inline uintptr_t get_insecure_context_return_addr() {
  // Original insecure return address should be above the flags:
  // XXX: This is untested!
  uintptr_t *ptr = (uintptr_t *)get_insecure_context_rsp();
  return ptr[1];
}

#endif // _STACK_SWITCH_
