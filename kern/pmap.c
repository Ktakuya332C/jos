#include <kern/pmap.h>
#include <inc/string.h>
#include <kern/kclock.h>

// These variables are set by i386_detect_memory()
size_t npages; // The amount of physical memory (in pages)
static size_t npages_basemem; // The amount of base memory (in pages)

// These variables are set in mem_init()
pde_t *kern_pgdir; // Kernel's initial page directory

/**** Detect machine's physical memory setup ****/

static int nvram_read(int r) {
  return mc146818_read(r) | (mc146818_read(r+1) << 8);
}

static void i386_detect_memory(void) {
  // Use CMOS callsto measure available memories
  // CMOS calls return results in KB
  size_t basemem = nvram_read(NVRAM_BASELO);
  size_t extmem = nvram_read(NVRAM_EXTLO);
  size_t ext16mem = nvram_read(NVRAM_EXT16LO) * 64;
  
  // Calculate the number of physical pages available
  size_t totalmem;
  if (ext16mem) {
    totalmem = 16 * 1024 + ext16mem;
  } else if (extmem) {
    totalmem = 1 * 1024 + extmem;
  } else {
    totalmem = basemem;
  }
  npages = totalmem / (PGSIZE / 1024);
  npages_basemem = basemem / (PGSIZE / 1024);
  
  cprintf("Available physical memory sizes\n");
  cprintf("-- Total memory   : %uKiB\n", totalmem);
  cprintf("-- Base memory    : %uKiB\n", basemem);
  cprintf("-- Extended memory: %uKiB\n", totalmem - basemem);
}

/**** Set up memory mappings above UTOP ****/

// Simple physical memory allocator used
// only while JOS is setting up its virtual memory system
static void* boot_alloc(uint32_t n) {
  static char *nextfree;
  char *result;
  
  // Initialize `nextfree` if this is the first time
  if (!nextfree) {
    extern char end[];
    nextfree = ROUNDUP((char*)end, PGSIZE);
  }
  
  // Allocate a chunk large enough to hold `n` bytes
  result = nextfree;
  nextfree += ROUNDUP(n, PGSIZE);
  
  return result;
}

void mem_init(void) {
  // Find out how much memory the machine has
  i386_detect_memory();
  
  // Create initial page directory
  kern_pgdir = (pde_t*)boot_alloc(PGSIZE);
  memset(kern_pgdir, 0, PGSIZE);
  
  // Recursively insert the page directory in itself as a page table
  kern_pgdir[PDX[UVPT]] = PADDR(kern_pgdir) | PTE_U | PTE_P;
  
  panic("Panic in mem_init function");
}
