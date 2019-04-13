#ifndef INC_MEMLAYOUT_H
#define INC_MEMLAYOUT_H

#ifndef __ASSEMBLER__
#include <inc/types.h>
#include <inc/mmu.h>
#endif // __ASSEMBLER__

// Global descriptor numbers
#define GD_KT 0x8 // kernel text
#define GD_KD 0x10 // kernel data
#define GD_UT 0x18 // user text
#define GD_UD 0x20 // user data
#define GD_TSS0 0x28 // Task segment selector for CPU 0

// All physical memory mapped at this address
#define KERNBASE 0xF0000000

// At IOPHYSMEM (640KB), there is a 384KB hole for I/O. From the kernel,
// IOPHYSMEM can be addressed at KERNBASE + IOPHYSMEM. The hole ends
// at physical address EXTPHYSMEM.
#define IOPHYSMEM 0x0A0000
#define EXTPHYSMEM 0x100000

// Kernel stack
#define KSTACKTOP KERNBASE
#define KSTKSIZE (8*PGSIZE) // Size of a kernel stack
#define KSTKGAP (8*PGSIZE) // Size of a kernel stack guard

// Memory-mapped IO
#define MMIOLIM (KSTACKTOP - PTSIZE)
#define MMIOBASE (MMIOLIM - PTSIZE)

#define ULIM (MMIOBASE)
#define UVPT (ULIM - PTSIZE) // User read-only virtual page table
#define UPAGES (UVPT - PTSIZE) // Read-only copies of the Page structures
#define UENVS (UPAGES - PTSIZE) // Read-only copies of the global env structures

#define UTOP UENVS // Top of user-accessible virtual memory
#define UXSTACKTOP UTOP // Top of one-page user exception stack
#define USTACKTOP (UTOP - 2*PGSIZE) // Top of normal user stack
#define UTEXT (2*PTSIZE) // Where user program generally begin
#define UTEMP ((void*)PTSIZE) // Used for temporally page mappings
// Used for temporally page mappgins for the user page-fault handler
#define PFTEMP (UTEMP - PTSIZE - PGSIZE)
// The location of the user-level STABS data structure
#define USTABDATA (PTSIZE / 2)

#ifndef __ASSEMBLER__
typedef uint32_t pte_t;
typedef uint32_t pde_t;

struct PageInfo {
  struct PageInfo *pp_link; // Next page on the free list
  uint16_t pp_ref; // Reference counter
};

#endif // __ASSEMBLER__

#endif // INC_MEMORYLAYOUT_H
