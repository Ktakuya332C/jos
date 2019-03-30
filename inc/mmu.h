#ifndef INC_MMU_H
#define INC_MMU_H

// Page directory and page table constants
#define NPDENTRIES 1024 // Number of Page Directory ENTRIES
#define NPTENTRIES 1024 // Number of Page Table ENTRIES
#define PGSIZE 4096 // The number of bytes in a page
#define PGSHIFT 12 // log2(PGSIZE)
#define PTSIZE (PGSIZE*NPTENTRIES) // The number of bytes in a page directory entry
#define PTSHIFT 22 // log2(PTSIZE)
#define PTXSHIFT 12 // Offset of PTX in a linear address
#define PDXSHIFT 22 // Offset of PDX in a linear address

// Control Register flags
#define CR0_PE 0x00000001	// Protection Enable
#define CR0_MP 0x00000002	// Monitor coProcessor
#define CR0_EM 0x00000004	// Emulation
#define CR0_TS 0x00000008	// Task Switched
#define CR0_ET 0x00000010	// Extension Type
#define CR0_NE 0x00000020	// Numeric Errror
#define CR0_WP 0x00010000	// Write Protect
#define CR0_AM 0x00040000	// Alignment Mask
#define CR0_NW 0x20000000	// Not Writethrough
#define CR0_CD 0x40000000	// Cache Disable
#define CR0_PG 0x80000000	// Paging

// Page directory/table entry flags
#define PTE_P 0x001	// Present
#define PTE_W 0x002	// Writeable
#define PTE_U 0x004	// User
#define PTE_PWT 0x008	// Write-Through
#define PTE_PCD 0x010	// Cache-Disable
#define PTE_A 0x020	// Accessed
#define PTE_D 0x040	// Dirty
#define PTE_PS 0x080	// Page Size
#define PTE_G 0x100	// Global

#endif // INC_MMU_H
