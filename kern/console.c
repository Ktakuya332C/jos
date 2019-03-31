#include <kern/console.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <inc/kbdreg.h>
#include <inc/string.h>

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
  switch (c & 0xFF) {
  case '\b':
    if (crt_pos > 0) {
      crt_pos--;
      crt_buf[crt_pos] = (c & ~0xFF) | ' ';
    }
    break;
  case '\n':
    crt_pos += CRT_COLS;
  case '\r':
    crt_pos -= (crt_pos % CRT_COLS);
    break;
  case '\t':
    cons_putc(' ');
    cons_putc(' ');
    cons_putc(' ');
    cons_putc(' ');
    break;
  default:
    crt_buf[crt_pos++] = c;
    break;
  }
  
  // if the cursor location exceeds the size of display
  // then remove the first line and move up all the rest of lines
  if (crt_pos >= CRT_SIZE) {
    memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
    int i;
    for (i=CRT_SIZE-CRT_COLS; i<CRT_SIZE; i++) {
      crt_buf[i] = 0x700 | ' ';
    }
    crt_pos -= CRT_COLS;
  }
  
  // Move cursor location
  outb(CGA_BASE, 0xE);
  outb(CGA_BASE+1, crt_pos >> 8);
  outb(CGA_BASE, 0xF);
  outb(CGA_BASE+1, crt_pos);
}

/**** Keyboard input code ****/

#define NO 0
#define SHIFT (1<<0)
#define CTL (1<<1)
#define ALT (1<<2)
#define CAPSLOCK (1<<3)
#define NUMLOCK (1<<4)
#define SCROLLLOCK (1<<5)
#define E0ESC (1<<6)

static uint8_t shiftcode[256] = {
  [0x1D] = CTL,
  [0x2A] = SHIFT,
  [0x36] = SHIFT,
  [0x38] = ALT,
  [0x9D] = CTL,
  [0xB8] = ALT
};

static uint8_t togglecode[256] = {
  [0x3A] = CAPSLOCK,
  [0x45] = NUMLOCK,
  [0x46] = SCROLLLOCK
};

static uint8_t normalmap[256] = {
  NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
  'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
  'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
  '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
  'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
  NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
  NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
  '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
  [0xC7] = KEY_HOME,
  [0x9C] = '\n' /*KP_Enter*/,
  [0xB5] = '/' /*KP_Div*/,
  [0xC8] = KEY_UP,
  [0xC9] = KEY_PGUP,
  [0xCB] = KEY_LF,
  [0xCD] = KEY_RT,
  [0xCF] = KEY_END,
  [0xD0] = KEY_DN,
  [0xD1] = KEY_PGDN,
  [0xD2] = KEY_INS,
  [0xD3] = KEY_DEL
};

static uint8_t shiftmap[256] =
{
  NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
  '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
  'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
  'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
  '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
  'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
  NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
  NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
  '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
  '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
  [0xC7] = KEY_HOME,
  [0x9C] = '\n' /*KP_Enter*/,
  [0xB5] = '/' /*KP_Div*/,
  [0xC8] = KEY_UP,
  [0xC9] = KEY_PGUP,
  [0xCB] = KEY_LF,
  [0xCD] = KEY_RT,
  [0xCF] = KEY_END,
  [0xD0] = KEY_DN,
  [0xD1] = KEY_PGDN,
  [0xD2] = KEY_INS,
  [0xD3] = KEY_DEL
};

#define C(x) (x - '@')

static uint8_t ctlmap[256] = {
  NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
  NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
  C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
  C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
  C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
  NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
  C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
  [0x97] = KEY_HOME,
  [0xB5] = C('/'),
  [0xC8] = KEY_UP,
  [0xC9] = KEY_PGUP,
  [0xCB] = KEY_LF,
  [0xCD] = KEY_RT,
  [0xCF] = KEY_END,
  [0xD0] = KEY_DN,
  [0xD1] = KEY_PGDN,
  [0xD2] = KEY_INS,
  [0xD3] = KEY_DEL
};

static uint8_t *charcode[4] = {
  normalmap,
  shiftmap,
  ctlmap,
  ctlmap
};

// Get data from keyboards and returns
//  int (>0) if input buffer of the keyboard stores normal characters
//  0 if input bufer of the keyboard stores unusual characters (like escape)
//  -1 if there is not more data to read
static int kbd_proc_data(void) {
  static uint32_t shift;

  // If there is no more data to read, then return -1
  uint8_t stat = inb(KBSTATP);
  if ((stat & KBS_DIB) == 0) return -1;
  if (stat & KBS_TERR) return -1;
  
  // Read data from the input buffer
  uint8_t data = inb(KBDATAP);
  if (data == 0xE0) {
    // E0 escape character
    shift |= E0ESC;
    return 0;
  } else if (data & 0x80) {
    // Key released
    data = (shift & E0ESC ? data : data & 0x7F);
    shift &= ~(shiftcode[data] | E0ESC);
    return 0;
  } else if (shift & E0ESC) {
    // Last character was an E0 escape; or with 0x80
    data |= 0x80;
    shift &= ~E0ESC;
  }
  
  shift |= shiftcode[data];
  shift ^= togglecode[data];
  
  int c = charcode[shift & (CTL | SHIFT)][data];
  if (shift & CAPSLOCK) {
    if ('a' <= c && c <= 'z') {
      c += 'A' - 'a';
    } else if ('A' <= c && c <= 'Z') {
      c += 'a' - 'A';
    }
  }
  
  // Reboot with Ctrl + Alt + Del
  if (!(~shift & (CTL | ALT)) && c == KEY_DEL) {
    outb(0x92, 0x3);
  }
  
  return c;
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

// Initialize the console devices
void cons_init(void) {
  cga_init();
  kbd_init();
}

// Output a character to the console
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

/**** High level console IO ****/

void cputchar(int c) {
  cons_putc(c);
}
