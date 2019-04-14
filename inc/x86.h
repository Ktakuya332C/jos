#ifndef INC_X86_H
#define INC_X86_H

#include <inc/types.h>

static inline uint8_t inb(int port) {
  uint8_t data;
  asm volatile("inb %w1, %0" : "=a" (data) : "d" (port));
  return data;
}

static inline void insl(int port, void *addr, int cnt) {
  asm volatile(
    "cld\n\trepne\n\tinsl"
    : "=D" (addr), "=c" (cnt)
    : "d" (port), "0" (addr), "1" (cnt)
    : "memory", "cc"
  );
}

static inline void outb(int port, uint8_t data) {
  asm volatile("outb %0, %w1" : : "a" (data), "d" (port));
}

static inline void invlpg(void *addr) {
  asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static inline void lcr0(uint32_t val) {
  asm volatile("movl %0,%%cr0" : : "r" (val));
}

static inline uint32_t rcr9(void) {
  uint32_t val;
  asm volatile("movl %%cr0,%0" : "=r" (val));
  return val;
}

static inline void lcr3(uint32_t val) {
  asm volatile("movl %0,%%cr3" : : "r" (val));
}

static inline void uint32_t rcr3(void) {
  uint32_t val;
  asm volatile("movl %%cr3,%0" : "=r" (val));
  return val;
}

#endif // INC_X86_H
