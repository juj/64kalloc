/* 64kalloc.h: Written by Jukka Jyl„nki, 2022, released to Public Domain.
	       No need to retain this notice or attribute back. */

// Borland Turbo C++ 3.0 quirk: must include <alloc.h> in the file that
// calls farmalloc(), otherwise farmalloc() will still compile and link
// successfully, but at runtime will silently fail and always return a
// null pointer.
#include <alloc.h> // farmalloc()
#include <dos.h> // FP_SEG(), FP_OFF()
#include "64kalloc.h"

// Converts a far ptr to a 20-bit linear address.
unsigned long calculate_linear_address(void far *ptr)
{
  return ((unsigned long)FP_SEG(ptr) << 4) + (unsigned long)FP_OFF(ptr);
}

// Returns nonzero if the given [ptr, ptr+size[ memory block resides within
// a 64KB page.
int is_64kb_boundary_aligned(void far *ptr, unsigned long size)
{
  // If allocating e.g. 2x64KB pages, they must line up with the
  // page boundary.
  if (size >= 65536UL)
    return (unsigned int)calculate_linear_address(ptr) == 0;
  else // otherwise, check that the data fits inside a single page.
    return (calculate_linear_address(ptr) & 0xFFFFUL) + size <= 65536UL;
}

// Offsets the allocated ptr to the beginning of the page-aligned portion.
void far *align_payload_to_64kb(void far *faralloced_ptr)
{
  unsigned long payload_segment = (calculate_linear_address(faralloced_ptr) + 15ul) >> 4ul;
  // Align the given pointer up to next multiple of 16 bytes.
  // (based on the precondition from function faralloc_inside_64kb_page(),
  //  doing this alignment will locate the 64KB aligned portion)
  // N.b. looks like MK_FP() macro in Turbo C++ is somehow not
  // preprocessor expansion safe (even with extra parentheses), so must
  // retain the variable definition above and not inline it into the macro.
  return MK_FP(payload_segment, 0);
}

// Allocates memory within a single 64KB block for Sound Blaster
// DMA access. See more details at the top of this file.
void far *faralloc_inside_64kb_page(unsigned long num_bytes)
{
  // DOS base memory area consists of up to 10x 64KB pages, so
  // the following define should be at least this 10 elements large.
  // If this define is set to a too small value, then this function
  // will be unable to find a suitable memory area, and will return
  // with a failed allocation. To be able to probe the whole
  // base memory address space, the size of this define should be so
  // large that MAX_PROBED_ALLOCS*num_bytes >= 640*1024.
  // If you are constrained on program stack space, this value can
  // be lowered, but be sure to have this >= 10.
#define MAX_PROBED_ALLOCS 256
  void far *ptr, far *consumed_page_ptrs[MAX_PROBED_ALLOCS];
  int num_consumed_pages = 0;

  // Probe test allocations until we succeed.
  while(num_consumed_pages < MAX_PROBED_ALLOCS)
  {
    // Internally it looks like that Borland Turbo C++ always returns
    // pointers that are quantized at a multiple of 16 bytes addresses,
    // but not necessarily aligned to 16 bytes. E.g. in one test
    // allocating series of random sized bytes would always return
    // pointers that have pointer values offset to 4 (mod 16).

    // In particular this means there is no way to "get lucky": Turbo C++
    // will never return back a pointer that would be exactly 64KB
    // aligned. To fix this, we allocate up to 15 bytes more, so that
    // we can align the allocated pointer up to the next multiple of 16,
    // which we hope to be an address that will fit the desired payload
    // inside a 64KB aligned page.
    ptr = farmalloc(num_bytes+15);

    // If the 16-byte aligned ptr resides within a single 64KB page, we
    // are done! Exit the loop to proceed to free up all probed allocations.
    if (is_64kb_boundary_aligned(align_payload_to_64kb(ptr), num_bytes))
      break;

    // We did not get lucky, so here is the strategy:
    // the memory just allocated straddles two 64KB pages ('lower' and
    // 'upper'). Therefore free this allocation, but immediately do a new
    // alloc, with a *soft* expectation that this new allocation will happen
    // to the same starting address as the old one. ("first fit") This new
    // alloc is sized so that it will consume the 'lower' page, hence the
    // next call to farmalloc() should reside on another (more pristine?)
    // 64KB page that hopefully will fit our 'new_bytes' block.

    // Because none of this is guaranteed, we perform these kinds of probe
    // allocations in a loop, until we have either exhausted all base
    // memory trying, or finally succeed.
    // After the actual allocation succeeds, we will free all the initial
    // 'probe' allocations back to the system.

    // In practice, if this funtion is called as the first thing
    // in the program, this loop is observed to iterate only once, as
    // the whole heap is completely unfragmented, and the second iteration
    // will already succeed.
    farfree(ptr);
    consumed_page_ptrs[num_consumed_pages++] = farmalloc(65520u - (unsigned int)(calculate_linear_address(ptr)));
    // Our alloc attempt failed, clear ptr to return failure if we ran
    // out of stack space for probe attempts.
    ptr = 0;
  }

  // Release all temporary claimed memory before returning
  // the (succeeded or failed) pointer back to the user.
  while(num_consumed_pages > 0)
    farfree(consumed_page_ptrs[--num_consumed_pages]);

  return ptr; // Returns 0 on failure
}
