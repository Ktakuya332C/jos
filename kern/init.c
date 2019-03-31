#include <inc/string.h>
#include <kern/console.h>

void i386_init(void) {
  extern char edata[], end[];
  
  // Clear uninitialized global data section (bss)
  memset(edata, 0, end - edata);
  
  // Initialize the console
  cons_init();
  
  // Put characters A and B
  cons_putc(0x41);
  cons_putc(0x42);
  
  while(1);
}
