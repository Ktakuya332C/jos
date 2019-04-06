#ifndef INC_TYPES_H
#define INC_TYPES_H

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

// size_t is used for memory object sizes.
typedef uint32_t size_t;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#endif // INC_TYPES_H
