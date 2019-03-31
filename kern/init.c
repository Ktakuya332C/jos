#include <inc/string.h>
#include <kern/console.h>

void i386_init(void) {
  extern char edata[], end[];
  
  // Clear uninitialized global data section (bss)
  memset(edata, 0, end - edata);
  
  // Initialize the console
  cons_init();
  
  // Input character A or B, and print it out
  int c = 0;
  while((c=cons_getc()) == 0);
  cons_putc(c);
  
  while(1);
}
