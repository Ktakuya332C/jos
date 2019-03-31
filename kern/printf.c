#include <inc/stdio.h>
#include <inc/stdarg.h>

static void putch(int ch, int* cnt) {
  cputchar(ch);
  (*cnt)++;
}

int vcprintf(const char* fmt, va_list ap) {
  int cnt = 0;
  vprintfmt((void*)putch, &cnt, fmt, ap);
  return cnt;
}

int cprintf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int cnt = vcprintf(fmt, ap);
  va_end(ap);
  return cnt;
}
