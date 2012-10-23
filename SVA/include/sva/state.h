/*===- state.h - SVA Interrupts   -----------------------------------------===
 * 
 *                     The LLVM Compiler Infrastructure
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 *
 * This header files defines functions and macros used by the SVA Execution
 * Engine for managing processor state.
 *
 *===----------------------------------------------------------------------===
 */

#ifndef _SVA_STATE_H
#define _SVA_STATE_H

#include <sys/types.h>

/* FreeBSD segment selector for kernel code segment */
#define KERNELCS 0x03

#include "sva/x86.h"
#if 0
#include "sva/util.h"
#include "sva/exceptions.h"
#endif

/* Processor privilege level */
typedef unsigned char priv_level_t;

/* Stack Pointer Typer */
typedef uintptr_t * sva_sp_t;

/*
 * Structure: icontext_t
 *
 * Description:
 *  This structure is what is saved by the Execution Engine when an interrupt,
 *  exception, or system call occurs.  It must ensure that all state that is
 *    (a) Used by the interrupted process, and
 *    (b) Potentially used by the kernel
 *  is saved and accessible until *the handler routine returns*.  On the
 *  x86_64, this means that we have to save *all* GPR's.
 *
 *  As the Execution Engine gets smarter, we might be able to skip saving some
 *  of these, or on hardware with shadow register sets, we might be able to
 *  forgo it at all.
 *
 * Notes:
 *  o) This structure *must* have a length equal to an even number of quad
 *     words.  The SVA interrupt handling code depends upon this behavior.
 */
typedef struct sva_icontext {
  /* Invoke Pointer */
  void * invokep;                     // 0x00

  /* Segment selector registers */
  unsigned short fs;                  // 0x08
  unsigned short gs;
  unsigned short es;
  unsigned short pad1;

  unsigned long rdi;                  // 0x10
  unsigned long rsi;                  // 0x18

  unsigned long rax;                  // 0x20
  unsigned long rbx;                  // 0x28
  unsigned long rcx;                  // 0x30
  unsigned long rdx;                  // 0x38

  unsigned long r8;                   // 0x40
  unsigned long r9;                   // 0x48
  unsigned long r10;                  // 0x50
  unsigned long r11;                  // 0x58
  unsigned long r12;                  // 0x60
  unsigned long r13;                  // 0x68
  unsigned long r14;                  // 0x70
  unsigned long r15;                  // 0x78

  /*
   * Keep this register right here.  We'll use it in assembly code, and we
   * place it here for easy saving and recovery.
   */
  unsigned long rbp;                  // 0x80

  /* Hardware trap number */
  unsigned long trapno;               // 0x88

  /*
   * These values are automagically saved by the x86_64 hardware upon an
   * interrupt or exception.
   */
  unsigned long code;                 // 0x90
  unsigned long rip;                  // 0x98
  unsigned long cs;                   // 0xa0
  unsigned long rflags;               // 0xa8
  unsigned long * rsp;                // 0xb0
  unsigned long ss;                   // 0xb8

  unsigned long skip;                 // 0xc0
  unsigned long start;                // 0xc8
} sva_icontext_t;

/*
 * Structure: sva_integer_state_t
 *
 * Description:
 *  This is all of the hardware state needed to represent an LLVM program's
 *  control flow, stack pointer, and integer registers.
 *
 * TODO:
 *  The stack pointer should probably be removed.
 */
typedef struct {
  /* Invoke Pointer */
  void * invokep;                     // 0x00

  /* Segment selector registers */
  unsigned short fs;                  // 0x08
  unsigned short gs;
  unsigned short es;
  unsigned short pad1;

  unsigned long rdi;                  // 0x10
  unsigned long rsi;                  // 0x18

  unsigned long rax;                  // 0x20
  unsigned long rbx;                  // 0x28
  unsigned long rcx;                  // 0x30
  unsigned long rdx;                  // 0x38

  unsigned long r8;                   // 0x40
  unsigned long r9;                   // 0x48
  unsigned long r10;                  // 0x50
  unsigned long r11;                  // 0x58
  unsigned long r12;                  // 0x60
  unsigned long r13;                  // 0x68
  unsigned long r14;                  // 0x70
  unsigned long r15;                  // 0x78

  /*
   * Keep this register right here.  We'll use it in assembly code, and we
   * place it here for easy saving and recovery.
   */
  unsigned long rbp;                  // 0x80

  /* Hardware trap number */
  unsigned long trapno;               // 0x88

  /*
   * These values are automagically saved by the x86_64 hardware upon an
   * interrupt or exception.
   */
  unsigned long code;                 // 0x90
  unsigned long rip;                  // 0x98
  unsigned long cs;                   // 0xa0
  unsigned long rflags;               // 0xa8
  unsigned long * rsp;                // 0xb0
  unsigned long ss;                   // 0xb8

  unsigned long skip;                 // 0xc0
  unsigned long start;                // 0xc8
} sva_integer_state_t;

typedef struct
{
  unsigned int state[7];
  unsigned int fp_regs[20];
} sva_fp_state_t;

/*
 * Structure: invoke_frame
 *
 * Description:
 *  This structure contains all of the information necessary to return
 *  state to the exceptional basic block when an unwind needs to be performed.
 */
struct invoke_frame
{
  unsigned int ebp;
  unsigned int ebx;
  unsigned int esi;
  unsigned int edi;
                                                                                
  struct invoke_frame * next;
  unsigned int cpinvoke;
};

/* The maximum number of interrupt contexts per CPU */
static const unsigned char maxIC = 32;

/*
 * Structure: CPUState
 *
 * Description:
 *  This is a structure containing the per-CPU state of each processor in the
 *  system.  We gather this here so that it's easy to find them from the %GS
 *  register.
 */
struct CPUState {
  /* Pointer to the currently available IC */
  sva_icontext_t * currentIC;

  /* Per-processor TSS segment */
  tss_t * tssp;

  /* New current interrupt Context */
  sva_icontext_t * newCurrentIC;

  void * padding;

  /*
   * Array of Interrupt Contexts for this CPU:
   * This field must be placed at a non-0x10 aligned boundary within the
   * CPUState structure.  This ensures that each interrupt context is not
   * aligned at a 16-byte boundary, which in tern prevents the x86 from
   * subtracting 3 64-bit words from the stack pointer on interrupts.
   *
   * This is because the x86 will first set the stack pointer to the value
   * in the IST, then decrement it, and *then* align it if necessary.
   */
  sva_icontext_t interruptContexts[maxIC + 1];
};

/*
 * Function: get_cpuState()
 *
 * Description:
 *  This function finds the CPU state for the current process.
 */
static inline struct CPUState *
getCPUState(void) {
  /*
   * Use an offset from the GS register to look up the processor CPU state for
   * this processor.
   */
  struct CPUState * cpustate;
  __asm__ __volatile__ ("movq %%gs:0x260, %0\n" : "=r" (cpustate));
  return cpustate;
}

/*
 * Intrinsic: sva_was_privileged()
 *
 * Description:
 *  This intrinsic flags whether the most recent interrupt context was running
 *  in a privileged state before the interrupt/exception occurred.
 *
 * Return value:
 *  true  - The processor was in privileged mode when interrupted.
 *  false - The processor was in user-mode when interrupted.
 */
static inline unsigned char
sva_was_privileged (void) {
  /*
   * Get the current processor's SVA state.
   */
  struct CPUState * cpuState = getCPUState();

  return (((cpuState->currentIC->cs) & KERNELCS) == KERNELCS);
}

#if 0
/* Prototypes for Execution Engine Functions */
extern unsigned char * sva_get_integer_stackp  (void * integerp);
extern void            sva_set_integer_stackp  (sva_integer_state_t * p, sva_sp_t sp);

extern void sva_push_function1 (void * integerp, void (*f)(int), int param);
extern void sva_push_syscall   (unsigned int sysnum, void * exceptp, void * fn);

extern void sva_load_kstackp (sva_sp_t);
extern sva_sp_t sva_save_kstackp (void);

extern void   sva_unwind      (sva_icontext_t * p);
extern unsigned int  sva_invoke      (unsigned int * retvalue,
                                       void *f, int arg1, int arg2, int arg3);

/*****************************************************************************
 * Global State
 ****************************************************************************/

extern void         sva_load_integer (void * p) __attribute__ ((regparm(0)));
extern unsigned int sva_save_integer (void * p) __attribute__ ((regparm(0)));
extern unsigned     sva_swap_integer  (unsigned int new,
                                       unsigned int * state)
                                      __attribute__ ((regparm(0)));
extern unsigned int sva_init_stack (unsigned char * sp, unsigned length,
                                    void * oip, void * f, unsigned int arg)
                                   __attribute__ ((regparm(0)));
extern void *       sva_declare_stack (void * p, unsigned size);
extern void         sva_release_stack (void * p);

/*****************************************************************************
 * Individual State Components
 ****************************************************************************/

/*
 * Intrinsic: sva_ipush_function0 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  icontext - A pointer to the exception handler saved state.
 *  newf     - The function to call.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
extern inline void
sva_ipush_function0 (void * icontext, void (*newf)(void))
{
  /* User Context Pointer */
  sva_icontext_t * ip = icontext;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Check the memory.
   */
  sva_check_memory_write (ip,      sizeof (sva_icontext_t));
  sva_check_memory_write (ip->esp, sizeof (unsigned int) * 1);

  /*
   * Verify that this interrupt context has a stack pointer.
   */
#if 0
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }
#endif

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ip->esp)) = ip->eip;

  /*
   * Set the return function to be the specificed function.
   */
  ip->eip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if SVA_SCLIMIT
  ip->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_ipush_function1 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  icontext - A pointer to the exception handler saved state.
 *  newf     - The function to call.
 *  param    - The parameter to send to the function.
 *
 * TODO:
 *  This currently only takes a function that takes a single integer
 *  argument.  Eventually, this should take any function.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
extern inline void
sva_ipush_function1 (void * icontext, void (*newf)(int), int param)
{
  /* User Context Pointer */
  sva_icontext_t * ep = icontext;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {
    /*
     * Check the memory.
     */
    sva_check_memory_write (ep, sizeof (sva_icontext_t));
    sva_check_memory_write (ep->esp, sizeof (unsigned int) * 2);

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(icontext))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /*
   * Push the one argument on to the user space stack.
   */
  *(--(ep->esp)) = param;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->esp)) = ep->eip;

  /*
   * Set the return function to be the specificed function.
   */
  ep->eip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if SVA_SCLIMIT
  ep->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

/*
 * Intrinsic: sva_ipush_function3 ()
 *
 * Description:
 *  This intrinsic modifies the user space process code so that the
 *  specified function was called with the given arguments.
 *
 * Inputs:
 *  icontext - A pointer to the exception handler saved state.
 *  newf     - The function to call.
 *  param    - The parameter to send to the function.
 *
 * TODO:
 *  This currently only takes a function that takes a single integer
 *  argument.  Eventually, this should take any function.
 *
 * NOTES:
 *  o This intrinsic could conceivably cause a memory fault (either by
 *    accessing a stack page that isn't paged in, or by overwriting the stack).
 *    This should be addressed at some point.
 */
extern inline void
sva_ipush_function3 (void * icontext, void (*newf)(int, int, int),
                      int p1, int p2, int p3)
{
  /* User Context Pointer */
  sva_icontext_t * ep = icontext;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  do
  {
    /*
     * Check the memory.
     */
    sva_check_memory_write (ep, sizeof (sva_icontext_t));
    sva_check_memory_write (ep->esp, sizeof (unsigned int) * 4);

    /*
     * Verify that this interrupt context has a stack pointer.
     */
    if (sva_is_privileged () && sva_was_privileged(icontext))
    {
      __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
      continue;
    }
    break;
  } while (1);

  /*
   * Push the arguments on to the user space stack.
   */
  *(--(ep->esp)) = p3;
  *(--(ep->esp)) = p2;
  *(--(ep->esp)) = p1;

  /*
   * Push the return PC pointer on to the stack.
   */
  *(--(ep->esp)) = ep->eip;

  /*
   * Set the return function to be the specificed function.
   */
  ep->eip = (unsigned int)newf;

  /*
   * Disable restrictions on system calls since we don't know where this
   * function pointer came from.
   */
#if SVA_SCLIMIT
  ep->sc_disabled = 0;
#endif

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");
  return;
}

extern inline unsigned char
sva_is_privileged  (void)
{
  unsigned int cs;
  __asm__ __volatile__ ("movl %%cs, %0\n" : "=r" (cs));
  return ((cs & 0x10) == 0x10);
}

/*
 * Intrinsic: sva_ialloca()
 *
 * Description:
 *  Allocate space on the current stack frame for an object of the specified
 *  size.
 */
extern inline void *
sva_ialloca (void * icontext, unsigned int size)
{
  sva_icontext_t * p = icontext;

  /*
   * Verify that this interrupt context has a stack pointer.
   */
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  /*
   * Perform the alloca.
   */
  return (p->esp -= ((size / 4) + 1));
}

/*
 * Intrinsic: sva_load_icontext()
 *
 * Description:
 *  This intrinsic takes state saved by the Execution Engine during an
 *  interrupt and loads it into the specified interrupt context buffer.
 */
extern inline void
sva_load_icontext (sva_icontext_t * icontextp, sva_integer_state_t * statep)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  sva_check_memory_read (statep, sizeof (sva_icontext_t));
  sva_check_memory_write (icontextp, sizeof (sva_icontext_t));

  /*
   * Verify that this interrupt context has a stack pointer.
   */
  if (sva_is_privileged () && sva_was_privileged(icontextp))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  /*
   * Currently, the interrupt context and integer state are one to one
   * identical.  This means that they can just be copied over.
   */
  __builtin_memcpy (icontextp, statep, sizeof (sva_icontext_t));

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");

  return;
}

/*
 * Intrinsic: sva_save_icontext()
 *
 * Description:
 *  This intrinsic takes state saved by the Execution Engine during an
 *  interrupt and saves it as an integer state structure.
 */
extern inline void
sva_save_icontext (sva_icontext_t * icontextp, sva_integer_state_t * statep)
{
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  sva_check_memory_read  (icontextp, sizeof (sva_icontext_t));
  sva_check_memory_write (statep,    sizeof (sva_icontext_t));

  /*
   * Verify that this interrupt context has a stack pointer.
   */
  if (sva_is_privileged () && sva_was_privileged(icontextp))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  /*
   * Currently, the interrupt context and integer state are one to one
   * identical.  This means that they can just be copied over.
   */
  __builtin_memcpy (statep, icontextp, sizeof (sva_icontext_t));

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");

  return;
}

/*
 * Intrinsic: sva_load_fp()
 *
 * Description:
 *  This intrinsic loads floating point state back on to the processor.
 */
extern inline void
sva_load_fp (void * buffer)
{
  const int ts = 0x00000008;
  unsigned int cr0;
  sva_fp_state_t * p = buffer;
  extern unsigned char sva_fp_used;
 
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  /*
   * Save the state of the floating point unit.
   */
  __asm__ __volatile__ ("frstor %0" : "=m" (p->state));

  /*
   * Mark the FPU has having been unused.  The first FP operation will cause
   * an exception into the Execution Engine.
   */
  __asm__ __volatile__ ("movl %%cr0, %0\n"
                        "orl  %1,    %0\n"
                        "movl %0,    %%cr0\n" : "=&r" (cr0) : "r" ((ts)));
  sva_fp_used = 0;

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");

  return;
}

/*
 * Intrinsic: sva_save_fp()
 *
 * Description:
 *  Save the processor's current floating point state into the specified
 *  buffer.
 *
 * Inputs:
 *  buffer - A pointer to the buffer in which to save the data.
 *  always - Only save state if it was modified since the last load FP state.
 */
extern inline int
sva_save_fp (void * buffer, int always)
{
  sva_fp_state_t * p = buffer;
  extern unsigned char sva_fp_used;

  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  if (always || sva_fp_used)
  {
    __asm__ __volatile__ ("fnsave %0" : "=m" (p->state) :: "memory");

    /*
     * Re-enable interrupts.
     */
    if (eflags & 0x00000200)
      __asm__ __volatile__ ("sti":::"memory");

    return 1;
  }

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");

  return 0;
}

/*
 * Intrinsic: sva_save_stackp ()
 *
 * Description:
 *  Return the current processor stack pointer to the caller.
 */
extern inline sva_sp_t
sva_save_stackp (void)
{
  unsigned int value;
  __asm__ ("movl %%esp, %0\n" : "=r" (value));
  return (void *)(value);
}

/*
 * Intrinsic: sva_load_stackp ()
 *
 * Description:
 *  Load on to the processor the stack pointer specified.
 */
extern inline void
sva_load_stackp  (sva_sp_t p)
{
  __asm__ __volatile__ ("movl %0, %%esp\n" :: "r" (p));
  return;
}

extern inline struct invoke_frame * gip;

/*
 * Intrinsic: sva_load_invoke()
 *
 * Description:
 *  Set the top of the invoke stack.
 */
extern inline void
sva_load_invoke (void * p)
{
  gip = p;
  return;
}

/*
 * Intrinsic: sva_save_invoke()
 *
 * Description:
 *  Save the current value of the top of the invoke stack.
 */
extern inline void *
sva_save_invoke (void)
{
  return gip;
}

extern inline unsigned int
sva_icontext_load_retvalue (void * icontext)
{
  return (((sva_icontext_t *)(icontext))->eax);
}

extern inline void
sva_icontext_save_retvalue (void * icontext, unsigned int value)
{
  (((sva_icontext_t *)(icontext))->eax) = value;
}

/*
 * Intrinsic: sva_get_icontext_stackp ()
 *
 * Description:
 *  Return the stack pointer that is saved within the specified interrupt
 *  context structure.
 */
extern inline unsigned char *
sva_get_icontext_stackp (void * icontext)
{
  /*
   * Verify that this interrupt context has a stack pointer.
   */
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }

  return (((sva_icontext_t *)icontext)->esp);
}

/*
 * Intrinsic: sva_set_icontext_stackp ()
 *
 * Description:
 *  Sets the stack pointer that is saved within the specified interrupt
 *  context structure.
 */
extern inline void
sva_set_icontext_stackp (void * icontext, void * stackp)
{
  /*
   * Verify that this interrupt context has a stack pointer.
   */
#if 0
  if (sva_is_privileged () && sva_was_privileged(icontext))
  {
    __asm__ __volatile__ ("int %0\n" :: "i" (sva_state_exception));
  }
#endif

  (((sva_icontext_t *)icontext)->esp) = stackp;
  return;
}

/*
 * Intrinsic: sva_iset_privileged ()
 *
 * Description:
 *  Change the state of the interrupt context to have come from an unprivileged
 *  state.
 */
extern inline void
sva_iset_privileged (void * icontext, unsigned char privileged)
{
  sva_icontext_t * p = icontext;
  /* Old interrupt flags */
  unsigned int eflags;

  /*
   * Disable interrupts.
   */
  __asm__ __volatile__ ("pushf; popl %0\n" : "=r" (eflags));

  if (privileged)
  {
    p->cs = 0x10;
    p->ds = 0x18;
    p->es = 0x18;
  }
  else
  {
    p->cs = 0x23;
    p->ds = 0x2B;
    p->es = 0x2B;
    p->ss = 0x2B;
  }

  /*
   * Re-enable interrupts.
   */
  if (eflags & 0x00000200)
    __asm__ __volatile__ ("sti":::"memory");

  return;
}

extern inline long long
sva_save_tsc (void)
{
  long long tsc;

  __asm__ __volatile__ ("rdtsc\n" : "=A" (tsc));
  return tsc;
}

extern inline void
sva_load_tsc (long long tsc)
{
  __asm__ __volatile__ ("wrmsr\n" :: "A" (tsc), "c" (0x10));
}

extern inline unsigned int
sva_invokememcpy (void * to, const void * from, unsigned long count)
{
  /* The invoke frame placed on the stack */
  struct invoke_frame frame;
                                                                                
  /* The invoke frame pointer */
  extern struct invoke_frame * gip;
                                                                                
  /* Return value */
  unsigned int ret = 0;
                                                                                
  /* Mark the frame as being used for a memcpy */
  frame.cpinvoke = 1;
  frame.next = gip;
                                                                                
  /* Make it the top invoke frame */
  gip = &frame;
                                                                                
  /* Perform the memcpy */
  __asm__ __volatile__ ("nop\nnop");
  __asm__ __volatile__ (
                        "movl $1f, %2\n"
                        "rep; movsl\n"
                        "movl $2, %0\n"
                        "movl %%edx, %%ecx\n"
                        "rep; movsb\n"
                        "1:\n"
                        "movl %%ecx, %1\n"
                        : "=m" (frame.cpinvoke), "=&a" (ret), "=m" (frame.esi)
                        : "D" (to),
                          "S" (from),
                          "c" (count / 4),
                          "d" (count & 0x3));
                                                                                
  /* Unlink the last invoke frame */
  gip = frame.next;
  return ret;
}
#endif
#endif
