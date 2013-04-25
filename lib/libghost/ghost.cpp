/*===- ghost.c - Ghost Compatibility Library ------------------------------===
 * 
 *                        Secure Virtual Architecture
 *
 * This file was developed by the LLVM research group and is distributed under
 * the University of Illinois Open Source License. See LICENSE.TXT for details.
 * 
 *===----------------------------------------------------------------------===
 *
 * This file defines compatibility functions to permit ghost applications to
 * use system calls.
 *
 *===----------------------------------------------------------------------===
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/un.h>

#include <cstdlib>

/* Size of traditional memory buffer */
static uintptr_t tradlen = 4096;

/* Buffer containing the traditional memory */
static unsigned char * tradBuffer;

/* Pointer into the traditional memory buffer stack */
static unsigned char * tradsp;

//
// Function: ghostinit()
//
// Description:
//  This function initializes the ghost run-time.  It should be called in a
//  program's main() function.
//
void
ghostinit (void) {
  tradBuffer = (unsigned char *) mmap(0, tradlen, PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
  if (tradBuffer == MAP_FAILED) {
    abort ();
  }

  tradsp = tradBuffer;
  return;
}

template<typename T>
static inline T *
allocateTradMem (unsigned char * & framePointer) {
  //
  // Save the current location of the traditional memory stack pointer.
  //
  framePointer = tradsp;

  //
  // Allocate memory on the traditional memory stack.
  //
  tradsp += sizeof (T);
  return (T *)tradsp;
}

template<typename T>
static inline T *
allocateTradMem (unsigned char * & framePointer, T* data) {
  //
  // Save the current location of the traditional memory stack pointer.
  //
  framePointer = tradsp;

  //
  // Allocate memory on the traditional memory stack.
  //
  tradsp += sizeof (T);

  //
  // Copy the data into the traditional memory.
  //
  T * copy = (T *)(tradsp);
  *copy = *data;
  return copy;
}

//////////////////////////////////////////////////////////////////////////////
// Wrappers for system calls
//////////////////////////////////////////////////////////////////////////////

int
accept (int s, struct sockaddr * addr, socklen_t * addrlen) {
  struct args {
    struct sockaddr_un addr;
    socklen_t addrlen;
  };

  unsigned char * framep;
  struct sockaddr * newaddr = allocateTradMem (framep, addr);
  socklen_t * newlen = allocateTradMem (framep, addrlen);
  accept (s, newaddr, newlen);
  return 0;
}

