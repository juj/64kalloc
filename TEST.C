#include <stdio.h>
#include <conio.h>

#include "64kalloc.h"

#define TEST_ALLOC_SIZE 65536

void main()
{
  void far *ptr;
  int randSize;
  srand(time(NULL));
  randSize = rand();
  printf("Disturbing heap with %d preallocated bytes. (%Fp)\n", randSize, farmalloc(randSize));

  for(;;) // Keep going as long as we are able to allocate memory.
  {
    ptr = faralloc_inside_64kb_page(TEST_ALLOC_SIZE);
    if (!ptr)
    {
      printf("Allocation failed.\n");
      getch();
      return;
    }
    ptr = align_payload_to_64kb(ptr);
    printf("%Fp (linear: 0x%lx): %lu bytes (%s 64KB page, offset=0x%x)\n",
      ptr,
      calculate_linear_address(ptr),
      TEST_ALLOC_SIZE,
      is_64kb_boundary_aligned(ptr, TEST_ALLOC_SIZE) ? "fits within" : "straddles",
      (unsigned int)(calculate_linear_address(ptr)));
  }
}
