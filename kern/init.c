#include <inc/string.h>

int mem_init = 0xffffffff;

void i386_init(void) {
  extern char edata[], end[];
  memset(edata, mem_init, end - edata);
  while(1);
}
