#include <inc/stdio.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *readline(const char *prompt) {
  if (prompt != NULL) cprintf("%s", prompt);
  int i = 0;
  int echoing = iscons(0);
  while(1) {
    int c = getchar();
    if (c < 0) {
      cprintf("read error: %e\n", c);
      return NULL;
    } else if ((c == '\b' || c == '\x7f') && i > 0) {
      if (echoing) cputchar('\b');
      i--;
    } else if (c >= ' ' && i < BUFLEN-1) {
      if (echoing) cputchar(c);
      buf[i++] = c;
    } else if (c == '\n' || c == '\r') {
      if (echoing) cputchar('\n');
      buf[i] = 0;
      return buf;
    }
  }
}