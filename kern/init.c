#include <inc/string.h>
#include <inc/stdio.h>
#include <kern/console.h>

void i386_init(void) {
  extern char edata[], end[];
  
  // Clear uninitialized global data section (bss)
  memset(edata, 0, end - edata);
  
  // Initialize the console
  cons_init();
  
  // Output string as a test
  cprintf("This is one: %d", 1);
}
