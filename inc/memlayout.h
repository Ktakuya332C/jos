#ifndef INC_MEMLAYOUT_H
#define INC_MEMLAYOUT_H

#ifndef __ASSEMBLER__
#include <inc/types.h>
#endif // __ASSEMBLER__

#define KERNBASE 0xF0000000

#ifndef __ASSEMBLER__
typedef uint32_t pte_t;
typedef uint32_t pde_t;
#endif // __ASSEMBLER__

#endif // INC_MEMORYLAYOUT_H
