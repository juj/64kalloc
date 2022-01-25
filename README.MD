64KALLOC
--------

This repository contains a far memory allocation routine suitable for DOS Borland Turbo C++ 3.0 for allocating memory inside a single 64KB memory page.

Example usage
-------------

Copy `64kalloc.c` and `64kalloc.h` to your project, then use it with

```c
#include "64kalloc.h"

int main()
{
  void *dmaMemory = faralloc_inside_64kb_page(65536);
  void *pageAlignedMem = align_payload_to_64kb(dmaMemory);

  // Pointer pageAlignedMem will now point to 65536 bytes aligned
  // to a 64KB page. This pointer can be passed to a DMA routine.

  // At deinit, call
  farfree(dmaMemory);
}
```

What is this about?
-------------------

The DMA controller on an IBM PC has a hardware limitation that the memory addresses it accesses within a single DMA operation cannot straddle a 64KB address boundary. Therefore when using the DMA controller e.g. for programming a Sound Blaster card, one must allocate memory that is suitably contained within a 64KB physical page.

However original Borland Turbo C/C++ compiler did not come with an API to allocate memory aligned in this manner, which left developers to implement manual workarounds. A typical workaround would be to allocate 2x the needed memory, so that a subportion of the allocated memory would always be guaranteed to reside within a 64KB page.

To avoid memory wastage from this 2x allocation, the excess portion of this overallocated memory buffer would then be attempted to be reused via a custom allocation assignment scheme.

This repository contains another method that works around this issue by coaxing the underlying memory allocator of Borland Turbo C/C++ to allocate suitably aligned memory with minimal wastage (only 16 bytes overhead).

More discussion about the topic
-------------------------------

To find more info about this programming challenge, see these two excellent resources:

- [Let's Code MS DOS 0x0A: Single Cycle SoundBlaster Playback!](https://www.youtube.com/watch?v=hn-9oL-ClCE&t=941s): A superb video from root42 on programming Sound Blaster from DOS,
- [VCFED: Reserve a 64k page in MS-DOS](https://forum.vcfed.org/index.php?threads/reserve-a-64k-page-in-ms-dos.79927/): Vintage Computer Federation programming forum discussion

How does it work?
-----------------

The idea with 64kalloc is to rely on "soft" guarantees that have been observed to occur with Borland Turbo C/C++ memory allocator, and leverage those guarantees to implement an allocation pattern that ends up with the allocator "magically" or "naturally" returning a suitably aligned pointer.

These two guarantees are:

1. Borland Turbo C/C++ allocator is what could be called **"left-flush"**. This means that when allocating a new pointer, the memory region that precedes the allocated region is in use. That is, memory is handed out bottom up from lower to higher addresses. (a somewhat sensible guarantee, given DOS real mode memory address scheme, and its use of the sbrk() model)

2. It looks like the Borland Turbo C/C++ allocator is either **"first-fit"**, or at least, if memory allocation is done as the first thing at program startup, when the heap is not yet fragmented, it will behave like first-fit. This means that the assumption

```c
void *ptr1 = farmalloc(x);
farfree(ptr1);
void *ptr2 = farmalloc(y); // where y <= x
assert(ptr1 == ptr2);
```

holds.

With these two assumptions, left-flush and first-fit, we can employ the following strategy:

1. Allocate the requested amount (<= 64KB) of memory with `farmalloc`.

2. The returned pointer will likely straddle the 64KB page, especially if allocating >= 32KB of memory.

3. If the allocation straddles a page, the memory will be in two parts A and B, with `AAA | BBB`, where
 - `|` denotes the 64KB page boundary split point,
 - since the allocator is assumed to be left-flush, the memory right before region `AAA` will be in use, and
 - the starting address of region `BBB` is suitably 64KB page aligned.

4. Free the memory allocation, but immediately allocate a new block of memory that consumes only the portion `AAA` of the previously allocated memory. Since the allocator is first-fit, Borland Turbo C/C++ will return the same memory pointer address as before, but one that covers only the region `AAA`.

Now since the allocator is left-flush first-fit, the next allocation should return a memory pointer that will start at address `BBB` that is exactly 64KB aligned. Perform the actual `farmalloc()` for the requested size. If the alignment checks out, proceed to next step 5. If something went wrong and we still did not get a suitably page-aligned memory, loop back to step 2 to repeat.

5. Finally, after we have succeeded with the actual allocation, `farfree()` all the temporary memory pointers that were allocated at step 4.

Testing this scheme in practice, there is a small complication to take into account with a a bit of bookkeeping structure that the memory allocator internally uses, and alignment/rounding to 16byte multiples, but with small accommodations for that, the above algorithm works well in practice. So well in fact, that when testing, I am unable to observe step 4 failing when the memory is being allocated as the very first thing in a program.

Notes
-----

- The allocator `faralloc_inside_64kb_page()` has an overallocation overhead of just 16 bytes. (this is the delta `align_payload_to_64kb(ptr) - ptr`)
- The allocator `faralloc_inside_64kb_page()` is not restricted to allocating only 64KB of memory. You can allocate e.g. 128KB, in which case a memory pointer aligned at the start of a 64KB physical boundary will be allocated, with the size of two consecutive 64KB pages.
- If you need to allocate two aligned pages of 2x32KB or 2x64KB each, it is more optimal to call `faralloc_inside_64kb_page()` only once with twice the size, rather than perform two individual allocations. This will result in less fragmentation of the heap.

Supported compilers
-------------------

This code has only been tested on Borland Turbo C++ 3.0 for DOS.

It is possible that the code does not work on other DOS compilers. I have not yet tested. However with small changes it should be possible to generalize code to work well on any other DOS C/C++ compiler.

If the code fails, feel free to open a GitHub issue about it to let the world know. (well, those three other retro DOS programmers still left on earth anyway)

If it works, feel free to open a PR as well to update this section. I may or may not reply, this is retro programming after all, just a pastime escape hatch from the real world.

Test code
---------

The file `TEST.C` contains an example code that stresses the allocator. Example output:

```
Disturbing heap with 26175 preallocated bytes. (0526:0000)
2000:0000 (linear: 0x20000): 65536 bytes (fits within 64KB page, offset=0x0)
4000:0000 (linear: 0x40000): 65536 bytes (fits within 64KB page, offset=0x0)
6000:0000 (linear: 0x60000): 65536 bytes (fits within 64KB page, offset=0x0)
8000:0000 (linear: 0x80000): 65536 bytes (fits within 64KB page, offset=0x0)
Allocation failed.
```

License
-------

The provided code is released to Public Domain. Copy-paste and mutate it to your needs, do not attribute back.. just go build that next awesome DOS game I can play on a retro PC ;)
