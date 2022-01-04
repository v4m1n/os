#pragma once
#include "stdint.h"
#include "stddef.h"


inline auto cpuid(uint64_t rax) {
  struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
  } regs;
  asm volatile("cpuid" : "=a"(regs.rax), "=b"(regs.rbx), "=c"(regs.rcx), "=d"(regs.rdx) : "a"(rax):"cc");
  return regs;
}

inline uint64_t rdmsr(uint32_t msr) {
  uint32_t edx, eax;
  asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
  return static_cast<uint64_t>(edx)<<32 | eax;
}

inline void wrmsr(uint32_t msr, uint64_t value) {
  asm volatile("wrmsr" :: "c"(msr), "a"(value), "d"(value>>32));
}

inline void clflush(uint64_t addr) {
  asm volatile("clflush [%0]" :: "r"(addr) : "memory");
}

inline void invlpg(uint64_t addr) {
  asm volatile("invlpg [%0]" :: "r"(addr) : "memory");
}
inline void outb(uint16_t port, uint8_t value) {
  asm volatile("outb dx, al" :: "d"(port), "a"(value) : "memory");
}
inline void outw(uint16_t port, uint16_t value) {
  asm volatile("outw dx, ax" :: "d"(port), "a"(value) : "memory");
}
inline void out(uint16_t port, uint32_t value) {
  asm volatile("out dx, eax" :: "d"(port), "a"(value) : "memory");
}
inline size_t xchg(volatile size_t &var, size_t value) {
  asm volatile("xchg %1, rax" : "=a"(value) : "m"(var), "a"(value));
  return value;
}
inline void atomic_inc(volatile size_t &var) {
  asm volatile("lock inc %0" :: "m"(var));
}
inline size_t atomic_fetch(volatile uint64_t &var) {
  size_t value;
  asm volatile("mov rax, %1" : "=a"(value) : "m"(var));
  return value;
}
inline void cbarrier() {
  asm volatile("" ::: "memory");
}
inline void barrier() {
  asm volatile("mfence" ::: "memory");
}
