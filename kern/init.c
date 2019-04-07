#include <inc/string.h>
#include <inc/stdio.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/pmap.h>

void i386_init(void) {
  extern char edata[], end[];
  
  // Clear uninitialized global data section (bss)
  memset(edata, 0, end - edata);
  
  // Initialize the console
  cons_init();
  
  // Initialize memory managements
  mem_init();
  
  // Drop into the kernel monitor
  while(1) monitor(NULL);
}
