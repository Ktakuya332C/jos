#ifndef INC_ASSERT_H
#define INC_ASSERT_H

#include <inc/stdio.h>

// kern/init.c
void _warn(const char*, int, const char*, ...);
void _panic(const char*, int, const char*, ...);

#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__);
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__);

#define assert(x) do { if (!(x)) panic("Assertion failed: %s", #x); } while(0)
#define static_assert(x) switch (x) case 0: case (x):

#endif // INC_ASSERT_H
