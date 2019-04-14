#include <kern/pmap.h>
#include <inc/mmu.h>
#include <inc/string.h>
#include <kern/kclock.h>

// These variables are set by i386_detect_memory()
size_t npages; // The amount of physical memory (in pages)
static size_t npages_basemem; // The amount of base memory (in pages)

// These variables are set in mem_init()
pde_t *kern_pgdir; // Kernel's initial page directory
struct PageInfo *pages; // Physical page state array
static struct PageInfo *page_free_list; // List of free physical pages

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

static void check_page_free_list(bool only_low_memory);
static void check_page_alloc(void);

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
  kern_pgdir[PDX(UVPT)] = PADDR(kern_pgdir) | PTE_U | PTE_P;
  
  // Allocate struct PageInfo for each page
  pages = (struct PageInfo*)boot_alloc(npages * sizeof(struct PageInfo));
  memset(pages, 0, npages * sizeof(struct PageInfo));
  
  // Set up the list of free physical pages
  page_init();
  check_page_free_list(1);
  check_page_alloc();
  
  panic("Panic in mem_init function");
}

// Initialize page structure and memory free list
void page_init(void) {
  for (size_t i=0; i<npages; i++) {
    physaddr_t phys_addr = i * PGSIZE;
    void* virt_addr = KADDR(phys_addr);
    
    // Physical page 0 is in use
    if (i == 0) continue;
    // Memory mapped IO
    if ((IOPHYSMEM <= phys_addr) && (phys_addr < EXTPHYSMEM)) continue;
    // Kernel and other data structures allocated by `boot_alloc`
    if ((EXTPHYSMEM <= phys_addr) && (virt_addr < boot_alloc(0))) {
      continue;
    }
    
    pages[i].pp_ref = 0;
    pages[i].pp_link = page_free_list;
    page_free_list = &pages[i];
  }
}

// Allocates a physical page
struct PageInfo* page_alloc(int alloc_flag) {
  if (page_free_list) {
    struct PageInfo *ret = page_free_list;
    page_free_list = page_free_list->pp_link;
    if (alloc_flag & ALLOC_ZERO) memset(page2kva(ret), 0, PGSIZE);
    return ret;
  } else {
    return NULL;
  }
}

// Return a page to the free list
void page_free(struct PageInfo *pp) {
  pp->pp_link = page_free_list;
  page_free_list = pp;
}

/**** Test functions ****/

// Check that the pages on the page_free_list are reasonable
static void check_page_free_list(bool only_low_memory) {
  struct PageInfo *pp;
  unsigned pdx_limit = only_low_memory ? 1 : NPDENTRIES;
  int nfree_basemem = 0, nfree_extmem = 0;
  char *first_free_page;
  
  if (!page_free_list)
    panic("'page_free_list' is a null pointer!");
  
  if (only_low_memory) {
    // Move pages with lower addresses first in the free
    // list, since entry_pgdir does not map all pages.
    struct PageInfo *pp1, *pp2;
    struct PageInfo **tp[2] = { &pp1, &pp2 };
    for (pp = page_free_list; pp; pp = pp->pp_link) {
      int pagetype = PDX(page2pa(pp)) >= pdx_limit;
      *tp[pagetype] = pp;
      tp[pagetype] = &pp->pp_link;
    }
    *tp[1] = 0;
    *tp[0] = pp2;
    page_free_list = pp1;
  }
  
  // if there's a page that shouldn't be on the free list,
  // try to make sure it eventually causes trouble.
  for (pp = page_free_list; pp; pp = pp->pp_link)
    if (PDX(page2pa(pp)) < pdx_limit)
      memset(page2kva(pp), 0x97, 128);
  
  first_free_page = (char *) boot_alloc(0);
  for (pp = page_free_list; pp; pp = pp->pp_link) {
    // check that we didn't corrupt the free list itself
    assert(pp >= pages);
    assert(pp < pages + npages);
    assert(((char *) pp - (char *) pages) % sizeof(*pp) == 0);
  
    // check a few pages that shouldn't be on the free list
    assert(page2pa(pp) != 0);
    assert(page2pa(pp) != IOPHYSMEM);
    assert(page2pa(pp) != EXTPHYSMEM - PGSIZE);
    assert(page2pa(pp) != EXTPHYSMEM);
    assert(page2pa(pp) < EXTPHYSMEM || (char *) page2kva(pp) >= first_free_page);
  
    if (page2pa(pp) < EXTPHYSMEM)
      ++nfree_basemem;
    else
      ++nfree_extmem;
  }
  
  assert(nfree_basemem > 0);
  assert(nfree_extmem > 0);
}

// Check page_alloc, page_free, page_init
static void check_page_alloc(void) {
  struct PageInfo *pp, *pp0, *pp1, *pp2;
  int nfree;
  struct PageInfo *fl;
  char *c;
  int i;

  if (!pages)
    panic("'pages' is a null pointer!");

  // check number of free pages
  for (pp = page_free_list, nfree = 0; pp; pp = pp->pp_link)
    ++nfree;

  // should be able to allocate three pages
  pp0 = pp1 = pp2 = 0;
  assert((pp0 = page_alloc(0)));
  assert((pp1 = page_alloc(0)));
  assert((pp2 = page_alloc(0)));

  assert(pp0);
  assert(pp1 && pp1 != pp0);
  assert(pp2 && pp2 != pp1 && pp2 != pp0);
  assert(page2pa(pp0) < npages*PGSIZE);
  assert(page2pa(pp1) < npages*PGSIZE);
  assert(page2pa(pp2) < npages*PGSIZE);

  // temporarily steal the rest of the free pages
  fl = page_free_list;
  page_free_list = 0;

  // should be no free memory
  assert(!page_alloc(0));

  // free and re-allocate?
  page_free(pp0);
  page_free(pp1);
  page_free(pp2);
  pp0 = pp1 = pp2 = 0;
  assert((pp0 = page_alloc(0)));
  assert((pp1 = page_alloc(0)));
  assert((pp2 = page_alloc(0)));
  assert(pp0);
  assert(pp1 && pp1 != pp0);
  assert(pp2 && pp2 != pp1 && pp2 != pp0);
  assert(!page_alloc(0));

  // test flags
  memset(page2kva(pp0), 1, PGSIZE);
  page_free(pp0);
  assert((pp = page_alloc(ALLOC_ZERO)));
  assert(pp && pp0 == pp);
  c = page2kva(pp);
  for (i = 0; i < PGSIZE; i++)
    assert(c[i] == 0);

  // give free list back
  page_free_list = fl;

  // free the pages we took
  page_free(pp0);
  page_free(pp1);
  page_free(pp2);

  // number of free pages should be the same
  for (pp = page_free_list; pp; pp = pp->pp_link)
    --nfree;
  assert(nfree == 0);
}
