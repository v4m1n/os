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
  asm volatile("out dx, al" :: "d"(port), "a"(value) : "memory");
}
inline void outw(uint16_t port, uint16_t value) {
  asm volatile("out dx, ax" :: "d"(port), "a"(value) : "memory");
}
inline void out(uint16_t port, uint32_t value) {
  asm volatile("out dx, eax" :: "d"(port), "a"(value) : "memory");
}
inline uint8_t inb(uint16_t port) {
  uint8_t value;
  asm volatile("in al, dx" : "=a"(value) : "d"(port) : "memory");
  return value;
}
inline uint16_t inw(uint16_t port) {
  uint16_t value;
  asm volatile("in ax, dx" : "=a"(value) : "d"(port) : "memory");
  return value;
}
inline uint32_t in(uint16_t port) {
  uint32_t value;
  asm volatile("in eax, dx" : "=a"(value) : "d"(port) : "memory");
  return value;
}
inline size_t xchg(volatile size_t &var, size_t value) {
  asm volatile("xchg %1, rax" : "=a"(value) : "m"(var), "a"(value));
  return value;
}
template <typename T>
inline T *getCPUStorage(const size_t offset) {
  T *value;
  asm volatile("swapgs \n mov %0, gs:[%1] \n swapgs" : "=r"(value) : "r"(offset));
  return value;
}
template <typename T>
inline void setCPUStorage(const size_t offset, T value) {
  asm volatile("swapgs \n mov gs:[%1], %0 \n swapgs" :: "r"(value), "r"(offset) :"memory");
}
inline void atomic_inc(volatile size_t &var) {
  asm volatile("lock inc %0" :: "m"(var));
}
inline uint32_t atomic_fetch(volatile uint32_t &var) {
  uint32_t value;
  asm volatile("mov eax, %1" : "=a"(value) : "m"(var));
  return value;
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
inline void hlt() {
  asm volatile("hlt" ::: "memory");
}
inline void setCR3(size_t val) {
  asm volatile("mov cr3, %0" :: "r"(val) : "memory");
}

namespace irq {
  inline bool getIF() {
    uint64_t flags;
    asm volatile("pushf\npop %0" :"=r"(flags):: "memory");
    return flags &0x200;
  }
  inline void enableInterrupts() { asm volatile("sti" ::: "memory"); }
  inline void disableInterrupts() { asm volatile("cli" ::: "memory"); }
}
