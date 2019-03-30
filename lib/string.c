#include <inc/string.h>

void* memset(void* v, int c, size_t n) {
  char* p;
  int m;
  p = v;
  m = n;
  while (--m >= 0) *p++ = c;
  return v;
}
