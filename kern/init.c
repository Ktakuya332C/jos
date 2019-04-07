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

// Variable panicstr contains argument to first call to panic
const char *panicstr;

// Panic is called on unresolvable fatal errors
// It prints a message and enters the kernel monitor
void _panic(const char *file, int line, const char *fmt, ...) {
  va_list ap;
  
  if (panicstr) goto dead;
  panicstr = fmt;
  
  asm volatile("cli; cld");
  
  va_start(ap, fmt);
  cprintf("Kernel panic at %s:%d ", file, line);
  vcprintf(fmt, ap);
  cprintf("\n");
  va_end(ap);
  
dead:
  while(1) monitor(NULL);
}

// Print out a warning message and return to the normal control flow
void _warn(const char *file, int line, const char *fmt, ...) {
  va_list ap;
  
  va_start(ap, fmt);
  cprintf("Kernel warning at %s:%d ", file, line);
  vcprintf(fmt, ap);
  cprintf("\n");
  va_end(ap);
}
