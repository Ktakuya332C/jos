#ifndef INC_STRING_H
#define INC_STRING_H

#include <inc/types.h>

int strlen(const char* s);
int strnlen(const char* s, size_t size);

void* memset(void* dst, int c, size_t len);
void* memmove(void* dst, const void* src, size_t n);

#endif // INC_STRING_H
