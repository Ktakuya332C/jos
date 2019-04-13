#ifndef KERN_PMAP_H
#define KERN_PMAP_H

#include <inc/memlayout.h>
#include <inc/assert.h>

// This macro takes a kernel virtual address
// and returns the corresponding physical address
#define PADDR(kva) _paddr(__FILE__, __LINE__, kva)
static inline phyaddr_t _paddr(const char *file, int line, void *kva) {
  if ((uint32_t)kva < KERNBASE) {
    _panic(file, line, "PADDR called with invalid kva %08lx", kva);
  }
  return (physaddr_t)kva - KERNBASE;
}

// This macro takes a physical address
// and returns the corresponding virtual address
#define KADDR(pa) _kaddr(__FILE__, __LINE__, pa)
static inline void* _kaddr(const char *file, int line, physaddr_t pa) {
  if (PGNUM(pa) >= npages) {
    _panic(file, line, "KADDR called with invalid pa %08lx", pa);
  }
  return (void*)(pa + KERNBASE);
}

void mem_init(void);

#endif // KERN_PMAP_H
