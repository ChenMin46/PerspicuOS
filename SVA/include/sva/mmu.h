/*===- mmu.h - SVA Execution Engine  =-------------------------------------===
 * 
 *                        Secure Virtual Architecture
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 */

#ifndef MMU_H
#define MMU_H

#include <sys/types.h>

/*
 *****************************************************************************
 * Define structures used in the SVA MMU interface.
 *****************************************************************************
 */
typedef uintptr_t cr3_t;
typedef uintptr_t pml4e_t;
typedef uintptr_t pdpte_t;
typedef uintptr_t pde_t;
typedef uintptr_t pte_t;

extern unsigned getPhysicalPage (void * v);

/*
 * Function: sva_mm_save_pgtable()
 *
 * Description:
 *  Get the current page table.
 */
static inline void *
sva_mm_save_pgtable (void)
{
  void * p;
  __asm__ __volatile__ ("movq %%cr3, %0\n" : "=r" (p));
  
  return p;
}

unsigned getPhysicalPage (void * v);
#endif

