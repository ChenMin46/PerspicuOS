/*===- init.c - SVA Execution Engine  ---------------------------------------===
 * 
 *                        Secure Virtual Architecture
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 *
 * This is code to initialize the SVA Execution Engine.  It is inherited from
 * the original SVA system.
 *
 *===----------------------------------------------------------------------===
 */

/*-
 * Copyright (c) 1989, 1990 William F. Jolitz
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  from: @(#)segments.h  7.1 (Berkeley) 5/9/91
 * $FreeBSD: release/9.0.0/sys/amd64/include/segments.h 227946 2011-11-24 18:44:14Z rstone $
 */

/*-
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: release/9.0.0/sys/amd64/include/cpufunc.h 223796 2011-07-05 18:42:10Z jkim $
 */

#include "sva/config.h"

#include <string.h>
#include <limits.h>

#include <sys/types.h>

extern int printf(const char *, ...);
extern void panic(const char *, ...);

void register_x86_interrupt (int number, void *interrupt, unsigned char priv);
void register_x86_trap (int number, void *trap);
#if 0
static void fptrap (void);
static void init_debug (void);
static void init_mmu (void);
static void init_fpu ();
#endif
static void init_dispatcher ();

/* Flags whether the FPU has been used */
unsigned char llva_fp_used = 0;

/* Default LLVA interrupt, exception, and system call handlers */
extern void default_interrupt (unsigned int number, void * icontext);

/* Map logical processor ID to an array in the SVA data structures */
struct procMap svaProcMap[numProcessors];

/*
 * Structure: interrupt_table
 *
 * Description:
 *  This is a table that contains the list of interrupt functions registered
 *  with the Execution Engine.  Whenever an interrupt occurs, one of these
 *  functions will be dispatched.
 *
 *  Note that we need one of these per processor.
 */
extern void * interrupt_table[256];

/*
 * Taken from FreeBSD: amd64/segments.h
 *
 * Gate descriptors (e.g. indirect descriptors, trap, interrupt etc. 128 bit)
 * Only interrupt and trap gates have gd_ist.
 */
struct  gate_descriptor {
  unsigned long gd_looffset:16; /* gate offset (lsb) */
  unsigned long gd_selector:16; /* gate segment selector */
  unsigned long gd_ist:3;   /* IST table index */
  unsigned long gd_xx:5;    /* unused */
  unsigned long gd_type:5;    /* segment type */
  unsigned long gd_dpl:2;   /* segment descriptor priority level */
  unsigned long gd_p:1;   /* segment descriptor present */
  unsigned long gd_hioffset:48 __attribute__ ((__packed__));  /* gate offset (msb) */
  unsigned long sd_xx1:32;
} __attribute__ ((packed));

/* Taken from FreeBSD: amd64/segments.h */
#define GSEL(s,r) (((s)<<3) | r)      /* a global selector */
#define GCODE_SEL 4 /* Kernel Code Descriptor */

/*
 * Structure: sva_idt
 *
 * Description:
 *  This is the x86 interrupt descriptor table.  We use it to hold all of the
 *  interrupt vectors internally within the Execution Engine.
 *
 *  Note that we need one of these per processor.
 */
static struct gate_descriptor sva_idt[256];

/* Taken from segments.h in FreeBSD */
static const unsigned int SDT_SYSIGT=14;  /* system 64 bit interrupt gate */
static const unsigned int SDT_SYSTGT=15;  /* system 64 bit trap gate */

void
sva_debug (void) {
  printf ("SVA: Debug!\n");
  return;
}

/*
 * Function: register_x86_interrupt()
 *
 * Description:
 *  Install the specified handler into the x86 Interrupt Descriptor Table (IDT)
 *  as an interrupt.
 *
 * Inputs:
 *  number    - The interrupt number.
 *  interrupt - A pointer to the interrupt handler.
 *  priv      - The x86_64 privilege level which can access this interrupt.
 *
 * Notes:
 *  This is based off of the amd64 setidt() code in FreeBSD.
 */
void
register_x86_interrupt (int number, void *interrupt, unsigned char priv) {
  /*
   * First determine which interrupt table we should be modifying.
   */
  struct gate_descriptor *ip = &sva_idt[number];

  /*
   * Add the entry into the table.
   */
  ip->gd_looffset = (uintptr_t)interrupt;
  ip->gd_selector = GSEL(GCODE_SEL, 0);
  ip->gd_ist = 3;
  ip->gd_xx = 0;
  ip->gd_type = SDT_SYSIGT;
  ip->gd_dpl = priv;
  ip->gd_p = 1;
  ip->gd_hioffset = ((uintptr_t)interrupt)>>16 ;

  return;
}

/*
 * Function: register_x86_trap()
 *
 * Description:
 *  Install the specified handler in the x86 Interrupt Descriptor Table (IDT)
 *  as a trap handler that can be called from privilege levels 0-3.
 *
 * Inputs:
 *  number  - The interrupt number.
 *  trap    - A function pointer to the system call handler.
 */
void
register_x86_trap (int number, void *trap) {
  /*
   * First determine which interrupt table we should be modifying.
   */
  struct gate_descriptor *ip = &sva_idt[number];

  /*
   * Add the entry into the table.
   */
  ip->gd_looffset = (uintptr_t)trap;
  ip->gd_selector = GSEL(GCODE_SEL, 3);
  ip->gd_ist = 3;
  ip->gd_xx = 0;
  ip->gd_type = SDT_SYSTGT;
  ip->gd_dpl = 3;
  ip->gd_p = 1;
  ip->gd_hioffset = ((uintptr_t)trap)>>16 ;

  return;
}

#if 0
/*
 * Function: fptrap()
 *
 * Description:
 *  Function that captures FP traps and flags use of the FP unit accordingly.
 */
static void
fptrap (void)
{
  const int ts = 0x00000008;
  int cr0;

  /*
   * Flag that the floating point unit has now been used.
   */
  llva_fp_used = 1;

  /*
   * Turn off the TS bit in CR0; this allows the FPU to proceed with floating
   * point operations.
   */
  __asm__ __volatile__ ("mov %%cr0, %0\n"
                        "andl  %1,  %0\n"
                        "mov %0, %%cr0\n" : "=&r" (cr0) : "r" (~(ts)));
}
#endif

/*
 * Function: init_procID()
 *
 * Description:
 *  Determine the APIC processor ID and map that to an available SVA logical
 *  processor ID.
 */
static void
init_procID (void) {
  /*
   * Use the CPUID instruction to get a local APIC2 ID for the processor.
   */
  unsigned int apicID;
  __asm__ __volatile__ ("movl $0xB, %%eax\ncpuid" : "=d" (apicID));

  /*
   * Find an available processor ID and use that.
   */
  for (unsigned index = 0; index < numProcessors; ++index) {
#if 1
    if (__sync_bool_compare_and_swap (&(svaProcMap[index].allocated), 0, 1)) {
#else
    if (!(svaProcMap[index].allocated)) {
#endif
      svaProcMap[index].allocated = 1;
      svaProcMap[index].apicID = apicID;
      return;
    }
  }

  return;
}

/*
 * Function: init_interrupt_table()
 *
 * Description:
 *  This function initializes the table of system software functions to call
 *  when an interrupt or trap occurs.  Since the system software hasn't set up
 *  any callback functions, we use a default handler that belongs to SVA.
 */
static void
init_interrupt_table (unsigned int procID) {
  for (int index = 0; index < 256; index++) {
    interrupt_table[index] = default_interrupt;
  }

  return;
}

/*
 * Function: init_idt()
 *
 * Description:
 *  Initialize the x86 Interrupt Descriptor Table (IDT) to some nice default
 *  values for the specified processor.
 *
 * Inputs:
 *  procID - The ID of the processor which should have its IDT initialized.
 */
static void
init_idt (unsigned int procID) {
  /* Argument to lidt/sidt taken from FreeBSD. */
  static struct region_descriptor {
    unsigned long rd_limit:16;    /* segment extent */
    unsigned long rd_base :64 __attribute__ ((packed));  /* base address  */
  } __attribute__ ((packed)) sva_idtreg;

  /* Kernel's idea of where the IDT is */
  extern void * idt;

  /*
   * Load our descriptor table on to the processor.
   */
  sva_idtreg.rd_limit = sizeof (struct gate_descriptor[256]) - 1;
  sva_idtreg.rd_base = (uintptr_t) &(sva_idt[0]);
  __asm__ __volatile__ ("lidt (%0)" : : "r" (&sva_idtreg));
  idt = (void *) sva_idtreg.rd_base;

  return;
}

#if 0
static void
init_debug (void)
{
  __asm__ ("movl $0, %eax\n"
           "movl %eax, %db0\n"
           "movl %eax, %db1\n"
           "movl %eax, %db2\n"
           "movl %eax, %db3\n"
           "movl %eax, %db6\n"
           "movl %eax, %db7\n");
  return;
}

/*
 * Functoin: init_mmu()
 *
 * Description:
 *  Initialize the i386 MMU.
 */
static void
init_mmu (void)
{
  const int pse   = (1 << 4);
  const int pge   = (1 << 7);
  const int tsd   = (1 << 2);
  const int pvi   = (1 << 1);
  const int de    = (1 << 3);
  const int pce   = (1 << 8);
  const int osfxr = ((1 << 9) | (1 << 10));
  int value;

  /*
   * Enable:
   *  PSE: Page Size Extension (i.e. large pages).
   *  PGE: Page Global Extension (i.e. global pages).
   *
   * Disable:
   *  TSD: Allow user mode applications to read the timestamp counter.
   *  PVI: Virtual Interrupt Flag in Protected Mode.
   *   DE: By disabling, allows for legacy debug register support for i386.
   *
   * We will assume that page size extensions and page global bit extensions
   * exist within the processor.  If they don't, you're in big trouble!
   */
  __asm__ __volatile__ ("mov %%cr4, %0\n"
                        "orl  %1, %0\n"
                        "andl %2, %0\n"
                        "mov %0, %%cr4\n"
                        : "=&r" (value)
                        : "r" (osfxr | pse | pge | pce),
                          "r" (~(pvi | de | tsd)));

  return;
}

/*
 * Function: init_fpu()
 *
 * Description:
 *  Initialize various things that needs to be initialized for the FPU.
 */
static void
init_fpu ()
{
  const int mp = 0x00000002;
  const int em = 0x00000004;
  const int ts = 0x00000008;
  int cr0;

  /*
   * Configure the processor so that the first use of the FPU generates an
   * exception.
   */
  __asm__ __volatile__ ("mov %%cr0, %0\n"
                        "andl  %1, %0\n"
                        "orl   %2, %0\n"
                        "mov %0, %%cr0\n"
                        : "=&r" (cr0)
                        : "r" (~(em)),
                          "r" (mp | ts));

  /*
   * Register the co-processor trap so that we know when an FP operation has
   * been performed.
   */
  llva_register_general_exception (0x7, fptrap);

  /*
   * Flag that the floating point unit has not been used.
   */
  llva_fp_used = 0;
  return;
}
#endif

/*
 * Intrinsic: sva_init_primary()
 *
 * Description:
 *  This routine initializes all of the information needed by the SVA
 *  Execution Engine.  We do things here like setting up the interrupt
 *  descriptor table.  Note that this should be called by the primary processor
 *  (the first one that starts execution on system boot).
 */
void
sva_init_primary () {
#if 0
  init_segs ();
  init_debug ();
#endif
  /* Initialize the processor ID */
  init_procID();

  /* Initialize the IDT of the primary processor */
  init_interrupt_table(0);
  init_idt (0);
  init_dispatcher ();

#if 0
  init_mmu ();
  init_fpu ();
  llva_reset_counters();
  llva_reset_local_counters();
#endif
}

/*
 * Intrinsic: sva_init_secondary()
 *
 * Description:
 *  This routine initializes all of the information needed by the SVA
 *  Execution Engine.  We do things here like setting up the interrupt
 *  descriptor table.  Note that this should be called by secondary processors.
 */
void
sva_init_secondary () {
#if 0
  init_segs ();
  init_debug ();
#endif

  /*
   * Initialize the IDT of the primary processor
   * FIXME: For now, we use the primary processor's IDT.  When we can, we
   * should have the kernel register whatever is in the primary IDT into the
   * other processor's IDTs.
   */
  init_idt (0);

#if 0
  init_interrupt_table(0);
  init_dispatcher ();
#endif
#if 0
  init_mmu ();
  init_fpu ();
  llva_reset_counters();
  llva_reset_local_counters();
#endif
}

#define REGISTER_EXCEPTION(number) \
  extern void trap##number(void); \
  register_x86_interrupt ((number),trap##number, 0);

#define REGISTER_INTERRUPT(number) \
  extern void interrupt##number(void); \
  register_x86_interrupt ((number),interrupt##number, 0);

extern void mem_trap14(void);
extern void mem_trap17(void);

static void
init_dispatcher ()
{
  /* Register the secure memory allocation and deallocation traps */
  extern void secmemtrap(void);
  extern void secfreetrap(void);
  extern void SVAbadtrap(void);

  /*
   * Register the bad trap handler for all interrupts and traps.
   */
  for (unsigned index = 0; index < 255; ++index) {
    register_x86_interrupt (index, SVAbadtrap, 0);
  }

  /*
   * Register the secure memory allocation and deallocation handlers.
   */
  register_x86_trap (0x7e, secfreetrap);
  register_x86_trap (0x7f, secmemtrap);

  /* Register general exception */
  REGISTER_EXCEPTION(0);
  REGISTER_EXCEPTION(1);
  REGISTER_EXCEPTION(2);
  REGISTER_EXCEPTION(3);
  REGISTER_EXCEPTION(4);
  REGISTER_EXCEPTION(5);
  REGISTER_EXCEPTION(6);
  REGISTER_EXCEPTION(7);
  REGISTER_EXCEPTION(8);
  REGISTER_EXCEPTION(9);
  REGISTER_EXCEPTION(10);
  REGISTER_EXCEPTION(11);
  REGISTER_EXCEPTION(12);
  REGISTER_EXCEPTION(13);
  REGISTER_EXCEPTION(14);   // Memory trap exception
  REGISTER_EXCEPTION(15);
  REGISTER_EXCEPTION(16);
  REGISTER_EXCEPTION(17);   // Memory trap exception
  REGISTER_EXCEPTION(18);
  REGISTER_EXCEPTION(19);
  REGISTER_EXCEPTION(20);
  REGISTER_EXCEPTION(21);
  REGISTER_EXCEPTION(22);
  REGISTER_EXCEPTION(23);
  REGISTER_EXCEPTION(24);
  REGISTER_EXCEPTION(25);
  REGISTER_EXCEPTION(26);
  REGISTER_EXCEPTION(27);
  REGISTER_EXCEPTION(28);
  REGISTER_EXCEPTION(29);
  REGISTER_EXCEPTION(30);
  REGISTER_EXCEPTION(31);

  /* Register re-routed I/O interrupts */
  REGISTER_INTERRUPT(32);
  REGISTER_INTERRUPT(33);
  REGISTER_INTERRUPT(34);
  REGISTER_INTERRUPT(35);
  REGISTER_INTERRUPT(36);
  REGISTER_INTERRUPT(37);
  REGISTER_INTERRUPT(38);
  REGISTER_INTERRUPT(39);
  REGISTER_INTERRUPT(40);
  REGISTER_INTERRUPT(41);
  REGISTER_INTERRUPT(42);
  REGISTER_INTERRUPT(43);
  REGISTER_INTERRUPT(44);
  REGISTER_INTERRUPT(45);
  REGISTER_INTERRUPT(46);
  REGISTER_INTERRUPT(47);
  REGISTER_INTERRUPT(48);

  REGISTER_INTERRUPT(49);
  REGISTER_INTERRUPT(57);
  REGISTER_INTERRUPT(65);
  REGISTER_INTERRUPT(73);
  REGISTER_INTERRUPT(89);
  REGISTER_INTERRUPT(121);
  REGISTER_INTERRUPT(137);
  REGISTER_INTERRUPT(153);
  REGISTER_INTERRUPT(161);

  /* Register SMP interrupts */
  REGISTER_INTERRUPT(239);
  REGISTER_INTERRUPT(251);
  REGISTER_INTERRUPT(252);
  REGISTER_INTERRUPT(253);
  REGISTER_INTERRUPT(254);
  REGISTER_INTERRUPT(255);
  return;
}

