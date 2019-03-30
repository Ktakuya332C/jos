#include <inc/mmu.h>
#include <inc/memlayout.h>

# RELOC maps a symbol x from its link address to its load address
#define RELOC(x) ((x) - KERNBASE)

.globl _start
_start = RELOC(entry)
.global entry
entry:
  # Setup a trivial page directory and page table
  movl $(RELOC(entry_pgdir)), %eax
  movl %eax, %cr3 # Load the physical address of entry_pgdir into cr3
  movl %cr0, %eax
  orl $(CR0_PE | CR0_PG | CR0_WP), %eax
  movl %eax, %cr0 # Turn on paging
  
  # Jump up above KERNBASE
  mov $relocated, %eax
  jmp *%eax

relocated:
  jmp relocated
