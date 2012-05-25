/*===- config.h - SVA Utilities --------------------------------------------===
 * 
 *                        Secure Virtual Architecture
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 *
 * This header file contains macros that can be used to configure the SVA
 * Execution Engine.
 *
 *===----------------------------------------------------------------------===
 */

#ifndef _SVA_CONFIG_H
#define _SVA_CONFIG_H

#include <limits.h>

/* Total number of processors supported by this SVA Execution Engine */
static const unsigned int numProcessors=4;

/* Structure for describing processors */
struct procMap {
  unsigned char allocated : 1;
  unsigned int apicID;
};

/*
 * Function: getProcessorID()
 *
 * Description:
 *  Determine the processor ID of the current processor.
 *
 * Inputs:
 *  None.
 *
 * Return value:
 *  An index value less than numProcessors that can be used to index into
 *  per-CPU SVA data structures.
 */
static unsigned int
getProcessorID() {
  /* Map logical processor ID to an array in the SVA data structures */
  extern struct procMap svaProcMap[numProcessors];

  /*
   * Use the CPUID instruction to get a local APIC2 ID for the processor.
   */
  unsigned int apicID;
  __asm__ __volatile__ ("movl $0xB, %%eax\ncpuid" : "=d" (apicID));

  /*
   * Convert the APIC2 ID into an SVA logical processor ID.
   */
  for (unsigned index = 0; index < numProcessors; ++index) {
    if ((svaProcMap[index].apicID == apicID) && (svaProcMap[index].allocated))
      return index;
  }

  /*
   * If we did not find it, then add it to the table.
   *
   * TODO: Fix the race condition here.
   */
  for (unsigned index = 0; index < numProcessors; ++index) {
    if (!(svaProcMap[index].allocated)) {
      svaProcMap[index].allocated = 1;
      svaProcMap[index].apicID = apicID;
      return index;
    }
  }

  return UINT_MAX;
}

#endif
