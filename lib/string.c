#include <inc/string.h>

int strlen(const char* s) {
  int n;
  for (n=0; *s != '\0'; s++) n++;
  return n;
}

int strnlen(const char* s, size_t size) {
  int n;
  for (n=0; size > 0 && *s != '\0'; s++, size--) n++;
  return n;
}

void* memset(void* v, int c, size_t n) {
  char* p;
  int m;
  p = v;
  m = n;
  while (--m >= 0) *p++ = c;
  return v;
}

void* memmove(void* dst, const void* src, size_t n) {
  const char* s;
  char* d;
  s = src;
  d = dst;
  if (s < d && s + n > d) {
    s += n;
    d += n;
    while (n-- > 0) *--d = *--s;
  } else {
    while (n-- > 0) *d++ = *s++;
  }
  return dst;
}
