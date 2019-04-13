#ifndef INC_TYPES_H
#define INC_TYPES_H

#ifndef NULL
#define NULL ((void*)0)
#endif

// Represents true or false values
typedef _Bool bool;
enum { false, true };

// Explicitly-sized versions of integer types
typedef __signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

// Pointer types
typedef int32_t intptr_t; // Virtual Address
typedef uint32_t uintptr_t; // Virtual Address
typedef uint32_t physaddr_t; // Physical Address

// Page numbers are 32 bits long
typedef uint32_t ppn_t;

// size_t is used for memory object sizes.
typedef uint32_t size_t;
// ssize_t is a signed version of size_t
typedef int32_t ssize_t;

// off_t is used for file offsets and lengths
typedef int32_t off_t;

// Efficient min and max operations
#define MIN(_a, _b) \
({ \
  typeof(_a) __a = (_a); \
  typeof(_b) __b = (_b); \
  __a <= __b ? __a : __b; \
})
#define MAX(_a, _b) \
({ \
  typeof(_a) = __a = (_a); \
  typeof(_b) = __b = (_b); \
  __a >= __b ? __a : __b; \
})

// Rounding operations
#define ROUNDDOWN(a, n) \
({ \
  uint32_t __a = (uint32_t)(a); \
  (typeof(a))(__a - __a % (n)); \
})
#define ROUNDUP(a, n) \
({ \
  uint32_t __n = (uint32_t)(n); \
  (typeof(a))(ROUNDDOWN((uint32_t)(a) + __n - 1, __n)); \
})

// Return the offset of `member` relative to the beginning of struct `type`
#define offsetof(type, member) ((size_t) (&((type*)0)->member))

// Return the size of array `a`
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#endif // INC_TYPES_H
