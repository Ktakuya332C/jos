#include <inc/string.h>

void i386_init(void) {
  extern char edata[], end[];
  memset(edata, 0, end - edata);
  while(1);
}
