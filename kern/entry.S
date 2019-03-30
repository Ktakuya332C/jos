#include <inc/memlayout.h>

# RELOC maps a symbol x from its link address to its load address
#define RELOC(x) ((x) - KERNBASE)

.globl _start
_start = RELOC(entry)
.global entry
entry:
  jmp entry
