#include <inc/x86.h>
#include <inc/elf.h>

#define SECTSIZE 512
#define ELFHDR ((struct Elf*)0x10000)

// Wait for the disk to be ready
void waitdisk(void) {
  while((inb(0x1F7) & 0xC0) != 0x40);
}

// Read 512 bytes (one sector) from the hard disk at `offset` to `dst`
// Each byte of the `offset` means the following
//   from 0 to 27 th: 28-bit logical block address (LBA) of the hard disk
//   28 th: drive number
//   29 th: Always 1
//   30 th: Need to be 1 if LBA
//   31 th: Always 1
void readsect(void *dst, uint32_t offset) {
  waitdisk();
  
  outb(0x1F2, 1);
  outb(0x1F3, offset);
  outb(0x1F4, offset >> 8);
  outb(0x1F5, offset >> 16);
  outb(0x1F6, (offset >> 24) | 0xE0);
  outb(0x1F7, 0x20);
  
  waitdisk();
  
  insl(0x1F0, dst, SECTSIZE / 4);
}

// Read all sectors that include at least a fraction of an elf segment
// specified by `offset` and `count` onto physical address `pa`
// This may end up reading more bytes than the one specified by `count`
void readseg(uint32_t pa, uint32_t count, uint32_t offset) {
  uint32_t end_pa = pa + count;
  
  pa &= ~(SECTSIZE - 1);
  offset = (offset / SECTSIZE) + 1;
  
  while (pa < end_pa) {
    readsect((void*)pa, offset);
    pa += SECTSIZE;
    offset++;
  }
}

// Read all segments of the kernel and execute it
void bootmain(void) {
  struct Proghdr *ph, *eph;
  
  // Read presumably sufficient number of sectors to memory
  readseg((uint32_t)ELFHDR, SECTSIZE * 8, 0);
  if (ELFHDR->e_magic != ELF_MAGIC) {
    // If the magic number is not correct, loop infinitely
    while(1);
  }
  
  // Load each program segment
  ph = (struct Proghdr*)((uint8_t*)ELFHDR + ELFHDR->e_phoff);
  eph = ph + ELFHDR->e_phnum;
  for(; ph < eph; ph++) {
    readseg(ph->p_pa, ph->p_memsz, ph->p_offset);
  }
  
  // Call the entry point from the elf header
  ((void (*)(void))(ELFHDR->e_entry))();
}
