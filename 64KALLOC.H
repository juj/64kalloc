#ifndef _64kalloc_h_
#define _64kalloc_h_

/* 64kalloc.h: Written by Jukka Jyl„nki, 2022, released to Public Domain.
	       No need to retain this notice or attribute back.

faralloc_inside_64kb_page(): Allocates memory inside a single 64KB page,
			     suitable for DOS Sound Blaster DMA use.

Example usage:

  void *dmaMemory = faralloc_inside_64kb_page(65536);
  void *pageAlignedMem = align_payload_to_64kb(dmaMemory);
  // Pointer pageAlignedMem will now point to 65536 bytes aligned
  // to a 64KB page. This pointer can be passed to a DMA routine.
  // At deinit, call
  farfree(dmaMemory);

The returned pointer will not directly be aligned to a 64KB
page. After a successful allocation, call align_payload_to_64kb()
on the returned pointer to compute an aligned pointer that will
be guaranteed to be located within a single 64KB block.
 - Call farfree() on the returned pointer to free the memory. Note that
   the pointer returned from align_payload_to_64kb() should not be
   passed to farfree().
 - Note: If you want to allocate 2*32KB or 2*64KB blocks, it is slightly
   more optimal to call this function only once with total
   size of 65536 bytes or 131072 bytes respectively. In the latter case,
   this function will return two suitably 64KB aligned blocks of memory
   on consecutive 64KB pages. If this large allocation fails, then
   you can manually fall back to performing two distinct allocations. */
void far *faralloc_inside_64kb_page(unsigned long num_bytes);

/* Given a pointer obtained via faralloc_inside_64kb_page(),
 returns a pointer region inside that allocated buffer that is
 guaranteed to reside within a single 64KB block. */
void far *align_payload_to_64kb(void far *faralloced_ptr);

// Returns the 20-bit linear address of a given far pointer.
unsigned long calculate_linear_address(void far *ptr);

// Returns nonzero if the given [ptr, ptr+size[ memory block resides within
// a 64KB page.
int is_64kb_boundary_aligned(void far *ptr, unsigned long size);

#endif // _64kalloc_h_
