#include <kern/pmap.h>
#include <kern/kclock.h>

// These variables are set by i386_detect_memory()
size_t npages; // The amount of physical memory (in pages)
static size_t npages_basemem; // The amount of base memory (in pages)

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
  
  cprintf("Availabe physical memory sizes\n");
  cprintf("-- Total memory   : %uKB\n", totalmem);
  cprintf("-- Base memory    : %uKB\n", basemem);
  cprintf("-- Extended memory: %uKB\n", totalmem - basemem);
}

/**** Set up memory mappings above UTOP ****/

void mem_init(void) {
  // Find out how much memory the machine has
  i386_detect_memory();
}
