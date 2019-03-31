#include <kern/console.h>
#include <inc/x86.h>
#include <inc/memlayout.h>

/**** Text-mode VGA display output ****/
static uint16_t* crt_buf;
static uint16_t crt_pos;

static void cga_init(void) {
  // Extract cursor location
  outb(CGA_BASE, 0xE);
  unsigned pos = inb(CGA_BASE+1) << 8;
  outb(CGA_BASE, 0xF);
  pos |= inb(CGA_BASE+1);
  
  // Initialize buffer and cursor location
  crt_buf = (uint16_t*)(KERNBASE + CGA_BUF);
  crt_pos = pos;
}

static void cga_putc(int c) {
  // If no attributes are given,
  // then use a default with black character on a white background
  if (!(c & ~0xFF)) c |= 0x700;
  
  // Put the character
  crt_buf[crt_pos++] = c;
  
  // Move cursor location
  outb(CGA_BASE, 0xE);
  outb(CGA_BASE+1, crt_pos >> 8);
  outb(CGA_BASE, 0xF);
  outb(CGA_BASE+1, crt_pos);
}

/**** General device-indepdent console code ****/
void cons_init(void) {
  cga_init();
}

void cons_putc(int c) {
  cga_putc(c);
}
