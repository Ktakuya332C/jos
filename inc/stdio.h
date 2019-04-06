#ifndef INC_STDIO_H
#define INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL ((void*)0)
#endif // NULL

// kern/console.c
void cputchar(int c);
int getchar(void);
int iscons(int fd);

// kern/printf.c
int cprintf(const char* fmt, ...);

// lib/printfmt.c
void vprintfmt(void (*putch)(int, void*), void *putdat, const char* fmt, va_list);

// lib/readline.c
char *readline(const char *prompt);

#endif // INC_STDIO_H
