#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char ** argv) {
  unsigned char * ptr = 0;
  unsigned long size = 0;
  unsigned index;
  unsigned sum = 0;

  /*
   * Check that we have the correct number of arguments.
   */
  if (argc < 2) {
    printf ("Usage: %s <number of bytes to allocate?\n", argv[0]);
    return -1;
  }

  /*
   * Get the number of bytes of secure memory to allocate.
   */
  size = strtoul (argv[1], NULL, 0);
  printf ("Secure Memory size: %lx\n", size);

  /*
   * Call the secure memory allocator.
   */
  __asm__ __volatile__ ("movq %1, %%rdi\nint $0x7f\n" : "=a" (ptr) : "r" (size));
  printf ("Secure Memory at %p\n", (void *)ptr);
  fflush (stdout);


  /*
   * Read from the secure memory.
   */
  printf ("Reading Secure Memory at %p\n", (void *)ptr);
  fflush (stdout);
  for (index = 0; index < size; ++index) {
    printf ("Address: %p\n", &(ptr[index]));
    fflush (stdout);
    sum += ptr[index];
  }
  printf ("%d\n", sum);

  /*
   * Write to the secure memory.
   */
  printf ("Writing Secure Memory at %p\n", (void *)ptr);
  fflush (stdout);
  for (index = 0; index < size; ++index) {
    printf ("Address: %p\n", &(ptr[index]));
    ptr[index] = 'c';
  }
  ptr[5] = 0;
  for (index = 0; index < size; ++index) {
    printf ("Address: %p %c\n", &(ptr[index]), ptr[index]);
  }

  /*
   * Call a system call which will try to read secure memory.
   */
  access ("/foo");

#if 0
  /*
   * Free the secure memory.
   */
  printf ("Freeing Secure Memory at %p\n", (void *)ptr);
  fflush (stdout);
  __asm__ __volatile__ ("int $0x7e\n" : : "D" (ptr), "S" (size));

  printf ("Done!\n");
#endif
  return 0;
}
