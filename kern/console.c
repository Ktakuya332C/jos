#include <kern/console.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <inc/kbdreg.h>

static void cons_intr(int (*proc)(void));

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

/**** Keyboard input code ****/

// Get data from keyboards and returns
//  int (>0) if input buffer of the keyboard stores normal characters
//  0 if input bufer of the keyboard stores unusual characters (like escape)
//  -1 if there is not more data to read
static int kbd_proc_data(void) {
  // If there is no more data to read, then return -1
  uint8_t stat = inb(KBSTATP);
  if ((stat & KBS_DIB) == 0) return -1;
  
  // Read data from the input buffer
  uint8_t data = inb(KBDATAP);
  if (data == 0x1E) {
    return 'A';
  } else if (data == 0x30) {
    return 'B';
  }
  return 0;
}

// Read all characters that were not yet read into the console buffer
void kbd_intr(void) {
  cons_intr(kbd_proc_data);
}

static void kbd_init(void) {}

/**** General device-indepdent console code ****/

#define CONSBUFSIZE 512
static struct {
  uint8_t buf[CONSBUFSIZE];
  uint32_t rpos; // Read position
  uint32_t wpos; // Write position
} cons;

// Store input data read by `proc` to the console buffer `buf`
// `buf` is a circular buffer that stores the maximum of CONSBUFSIZE bytes.
void cons_intr(int (*proc)(void)) {
  int c;
  while ((c=(*proc)()) != -1) {
    if (c == 0) continue;
    cons.buf[cons.wpos++] = c;
    if (cons.wpos == CONSBUFSIZE) cons.wpos = 0;
  }
}

void cons_init(void) {
  cga_init();
  kbd_init();
}

void cons_putc(int c) {
  cga_putc(c);
}

// Read all characters that were not read yet to the console buffer
// and returns the first one character from the buffer.
int cons_getc(void) {
  kbd_intr();
  int c;
  if (cons.rpos != cons.wpos) {
    c = cons.buf[cons.rpos++];
    if (cons.rpos == CONSBUFSIZE) cons.rpos = 0;
    return c;
  }
  return 0;
}
