#include <kern/pmap.h>
#include <inc/env.h>
#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>

#include <kern/env.h>
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

static void boot_map_region(
    pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm);
static void check_page_free_list(bool only_low_memory);
static void check_page_alloc(void);
static void check_page(void);
static void check_kern_pgdir(void);
static void check_page_installed_pgdir(void);

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

  // Allocate an array of struct Env
  envs = (struct Env*)boot_alloc(NENV * sizeof(struct Env));
  
  // Set up the list of free physical pages
  page_init();
  check_page_free_list(1);
  check_page_alloc();
  check_page();
  
  // Map `pages` read-only by the user at linear address UPAGES
  boot_map_region(kern_pgdir, UPAGES, PTSIZE, PADDR(pages), PTE_U);
  
  // Map `envs` read-only by the user at linear address UENVS
  boot_map_region(kern_pgdir, UENVS, PTSIZE, PADDR(envs), PTE_U);
  
  // Map `bootstack` writable only by the kernel at linear address KSTACKTOP
  boot_map_region(
      kern_pgdir, KSTACKTOP-KSTKSIZE, KSTKSIZE, PADDR(bootstack), PTE_W);
  
  // Map virtual adresses [KERNBASE, 2**32)
  // to physical address [0, 2**32 - KERNBASE]
  // writable only by the kernel
  boot_map_region(kern_pgdir, KERNBASE, -KERNBASE, 0, PTE_W);
  check_kern_pgdir();
  
  // Switch from the minimal entry page directory to the full kern_pgdir
  lcr3(PADDR(kern_pgdir));
  check_page_free_list(0);
  
  // Reset cr0 bit flags
  uint32_t cr0 = rcr0();
  cr0 |= CR0_PE|CR0_PG|CR0_AM|CR0_WP|CR0_NE|CR0_MP;
  cr0 &= ~(CR0_TS|CR0_EM);
  lcr0(cr0);
  
  // Some more checks
  check_page_installed_pgdir();
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
    ret->pp_link = NULL;
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

// Given `pgdir`, a pointer to a page directory, pgdir_walk returns
// a pointer to the page table entry (PTE) for linear address `va
pde_t* pgdir_walk(pde_t *pgdir, const void *va, int create) {
  int dindex = PDX(va);
  if (!(pgdir[dindex] & PTE_P)) {
    if (create) {
      struct PageInfo *pg = page_alloc(ALLOC_ZERO);
      if (!pg) return NULL;
      pg->pp_ref++;
      pgdir[dindex] = page2pa(pg) | PTE_P | PTE_U | PTE_W;
    } else {
      return NULL;
    }
  }
  pte_t *p = KADDR(PTE_ADDR(pgdir[dindex]));
  int tindex = PTX(va);
  return p + tindex;
}

// Map [va, va+size) of virtual address space
// to physical address space [pa, pa+size)
// Size is a multiple of PGSIZE, and va and pa are both page-aligned
// This function is only intended to set up the static mappings above UTOP
static void boot_map_region(
    pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm) {
  size_t mapped = 0;
  while(mapped < ROUNDUP(size, PGSIZE)) {
    pte_t *pte = pgdir_walk(pgdir, (void*)va, 1);
    if (!pte) panic("Out of memory");
    *pte = pa | perm | PTE_P;
    
    mapped += PGSIZE;
    va += PGSIZE;
    pa += PGSIZE;
  }
}

// Return the page mapped at virtual address `va`
// If `pte_store` is not zero,
// then we store the address of the pte for this page in it
struct PageInfo* page_lookup(pde_t *pgdir, void *va, pte_t **pte_store) {
  pte_t *pte = pgdir_walk(pgdir, va, 0);
  if (!pte || !(*pte & PTE_P)) return NULL;
  if (pte_store) *pte_store = pte;
  return pa2page(PTE_ADDR(*pte));
}

// Decrement the reference count on page `pp`
// If there is no more refs, freeing the page
void page_decref(struct PageInfo *pp) {
  if (--pp->pp_ref == 0) page_free(pp);
}

// Invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor
void tlb_invalidate(pde_t *pgdir, void *va) {
  invlpg(va);
}

// Unmap the physical page at virtual address `va`
// If there is no physical page at that address, this function does nothing
void page_remove(pde_t *pgdir, void *va) {
  pte_t *pte;
  struct PageInfo *pg = page_lookup(pgdir, va, &pte);
  if (!pg || !(*pte & PTE_P)) return;
  page_decref(pg);
  *pte = 0;
  tlb_invalidate(pgdir, va);
}

// Map the physical page `pp` at virtual addrss `va`
int page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm) {
  pte_t *pte = pgdir_walk(pgdir, va, 1);
  if (!pte) return -E_NO_MEM;
  pp->pp_ref++;
  if (*pte & PTE_P) page_remove(pgdir, va);
  *pte = page2pa(pp) | perm | PTE_P;
  return 0;
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

// This function returns the physical address of the page containing `va`
static physaddr_t check_va2pa(pde_t *pgdir, uintptr_t va) {
  pte_t *p;
  pgdir = &pgdir[PDX(va)];
  if (!(*pgdir & PTE_P)) {
    return ~0;
  }
  p = (pte_t*) KADDR(PTE_ADDR(*pgdir));
  if (!(p[PTX(va)] & PTE_P)) {
    return ~0;
  }
  return PTE_ADDR(p[PTX(va)]);
}

// Check page_insert, page_remove
static void check_page(void) {
  struct PageInfo *pp, *pp0, *pp1, *pp2;
  struct PageInfo *fl;
  pte_t *ptep, *ptep1;
  void *va;
  int i;

  // should be able to allocate three pages
  pp0 = pp1 = pp2 = 0;
  assert((pp0 = page_alloc(0)));
  assert((pp1 = page_alloc(0)));
  assert((pp2 = page_alloc(0)));

  assert(pp0);
  assert(pp1 && pp1 != pp0);
  assert(pp2 && pp2 != pp1 && pp2 != pp0);

  // temporarily steal the rest of the free pages
  fl = page_free_list;
  page_free_list = 0;

  // should be no free memory
  assert(!page_alloc(0));

  // there is no page allocated at address 0
  assert(page_lookup(kern_pgdir, (void *) 0x0, &ptep) == NULL);

  // there is no free memory, so we can't allocate a page table
  assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) < 0);

  // free pp0 and try again: pp0 should be used for page table
  page_free(pp0);
  assert(page_insert(kern_pgdir, pp1, 0x0, PTE_W) == 0);
  assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
  assert(check_va2pa(kern_pgdir, 0x0) == page2pa(pp1));
  assert(pp1->pp_ref == 1);
  assert(pp0->pp_ref == 1);

  // should be able to map pp2 at PGSIZE because pp0 is already allocated for page table
  assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
  assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
  assert(pp2->pp_ref == 1);

  // should be no free memory
  assert(!page_alloc(0));

  // should be able to map pp2 at PGSIZE because it's already there
  assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
  assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
  assert(pp2->pp_ref == 1);

  // pp2 should NOT be on the free list
  // could happen in ref counts are handled sloppily in page_insert
  assert(!page_alloc(0));

  // check that pgdir_walk returns a pointer to the pte
  ptep = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(PGSIZE)]));
  assert(pgdir_walk(kern_pgdir, (void*)PGSIZE, 0) == ptep+PTX(PGSIZE));

  // should be able to change permissions too.
  assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W|PTE_U) == 0);
  assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp2));
  assert(pp2->pp_ref == 1);
  assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U);
  assert(kern_pgdir[0] & PTE_U);

  // should be able to remap with fewer permissions
  assert(page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W) == 0);
  assert(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_W);
  assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));

  // should not be able to map at PTSIZE because need free page for page table
  assert(page_insert(kern_pgdir, pp0, (void*) PTSIZE, PTE_W) < 0);

  // insert pp1 at PGSIZE (replacing pp2)
  assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W) == 0);
  assert(!(*pgdir_walk(kern_pgdir, (void*) PGSIZE, 0) & PTE_U));

  // should have pp1 at both 0 and PGSIZE, pp2 nowhere, ...
  assert(check_va2pa(kern_pgdir, 0) == page2pa(pp1));
  assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
  // ... and ref counts should reflect this
  assert(pp1->pp_ref == 2);
  assert(pp2->pp_ref == 0);

  // pp2 should be returned by page_alloc
  assert((pp = page_alloc(0)) && pp == pp2);

  // unmapping pp1 at 0 should keep pp1 at PGSIZE
  page_remove(kern_pgdir, 0x0);
  assert(check_va2pa(kern_pgdir, 0x0) == ~0);
  assert(check_va2pa(kern_pgdir, PGSIZE) == page2pa(pp1));
  assert(pp1->pp_ref == 1);
  assert(pp2->pp_ref == 0);

  // test re-inserting pp1 at PGSIZE
  assert(page_insert(kern_pgdir, pp1, (void*) PGSIZE, 0) == 0);
  assert(pp1->pp_ref);
  assert(pp1->pp_link == NULL);

  // unmapping pp1 at PGSIZE should free it
  page_remove(kern_pgdir, (void*) PGSIZE);
  assert(check_va2pa(kern_pgdir, 0x0) == ~0);
  assert(check_va2pa(kern_pgdir, PGSIZE) == ~0);
  assert(pp1->pp_ref == 0);
  assert(pp2->pp_ref == 0);

  // so it should be returned by page_alloc
  assert((pp = page_alloc(0)) && pp == pp1);

  // should be no free memory
  assert(!page_alloc(0));

  // forcibly take pp0 back
  assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
  kern_pgdir[0] = 0;
  assert(pp0->pp_ref == 1);
  pp0->pp_ref = 0;

  // check pointer arithmetic in pgdir_walk
  page_free(pp0);
  va = (void*)(PGSIZE * NPDENTRIES + PGSIZE);
  ptep = pgdir_walk(kern_pgdir, va, 1);
  ptep1 = (pte_t *) KADDR(PTE_ADDR(kern_pgdir[PDX(va)]));
  assert(ptep == ptep1 + PTX(va));
  kern_pgdir[PDX(va)] = 0;
  pp0->pp_ref = 0;

  // check that new page tables get cleared
  memset(page2kva(pp0), 0xFF, PGSIZE);
  page_free(pp0);
  pgdir_walk(kern_pgdir, 0x0, 1);
  ptep = (pte_t *) page2kva(pp0);
  for(i=0; i<NPTENTRIES; i++)
    assert((ptep[i] & PTE_P) == 0);
  kern_pgdir[0] = 0;
  pp0->pp_ref = 0;

  // give free list back
  page_free_list = fl;

  // free the pages we took
  page_free(pp0);
  page_free(pp1);
  page_free(pp2);
}

// Checks the kernel part of virtual address space
// has been set up roughly correctly
static void check_kern_pgdir(void) {
  uint32_t i, n;
  pde_t *pgdir;

  pgdir = kern_pgdir;

  // check pages array
  n = ROUNDUP(npages*sizeof(struct PageInfo), PGSIZE);
  for (i = 0; i < n; i += PGSIZE)
    assert(check_va2pa(pgdir, UPAGES + i) == PADDR(pages) + i);

  // check envs array (new test for lab 3)
  n = ROUNDUP(NENV*sizeof(struct Env), PGSIZE);
  for (i = 0; i < n; i += PGSIZE)
    assert(check_va2pa(pgdir, UENVS + i) == PADDR(envs) + i);

  // check phys mem
  for (i = 0; i < npages * PGSIZE; i += PGSIZE)
    assert(check_va2pa(pgdir, KERNBASE + i) == i);

  // check kernel stack
  for (i = 0; i < KSTKSIZE; i += PGSIZE)
    assert(check_va2pa(pgdir, KSTACKTOP - KSTKSIZE + i) == PADDR(bootstack) + i);
  assert(check_va2pa(pgdir, KSTACKTOP - PTSIZE) == ~0);

  // check PDE permissions
  for (i = 0; i < NPDENTRIES; i++) {
    switch (i) {
    case PDX(UVPT):
    case PDX(KSTACKTOP-1):
    case PDX(UPAGES):
    case PDX(UENVS):
      assert(pgdir[i] & PTE_P);
      break;
    default:
      if (i >= PDX(KERNBASE)) {
        assert(pgdir[i] & PTE_P);
        assert(pgdir[i] & PTE_W);
      } else
        assert(pgdir[i] == 0);
      break;
    }
  }
}

// Check page_insert, page_remove, &c, with an installed kern_pgdir
static void check_page_installed_pgdir(void) {
  struct PageInfo *pp0, *pp1, *pp2;

  // check that we can read and write installed pages
  pp1 = pp2 = 0;
  assert((pp0 = page_alloc(0)));
  assert((pp1 = page_alloc(0)));
  assert((pp2 = page_alloc(0)));
  page_free(pp0);
  memset(page2kva(pp1), 1, PGSIZE);
  memset(page2kva(pp2), 2, PGSIZE);
  page_insert(kern_pgdir, pp1, (void*) PGSIZE, PTE_W);
  assert(pp1->pp_ref == 1);
  assert(*(uint32_t *)PGSIZE == 0x01010101U);
  page_insert(kern_pgdir, pp2, (void*) PGSIZE, PTE_W);
  assert(*(uint32_t *)PGSIZE == 0x02020202U);
  assert(pp2->pp_ref == 1);
  assert(pp1->pp_ref == 0);
  *(uint32_t *)PGSIZE = 0x03030303U;
  assert(*(uint32_t *)page2kva(pp2) == 0x03030303U);
  page_remove(kern_pgdir, (void*) PGSIZE);
  assert(pp2->pp_ref == 0);

  // forcibly take pp0 back
  assert(PTE_ADDR(kern_pgdir[0]) == page2pa(pp0));
  kern_pgdir[0] = 0;
  assert(pp0->pp_ref == 1);
  pp0->pp_ref = 0;

  // free the pages we took
  page_free(pp0);
}
