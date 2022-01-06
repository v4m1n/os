#pragma once
#include "stdint.h"

struct Thread;

namespace thrd {
  struct registers {
    uint64_t gs_base;
    uint64_t fs_base;
    uint64_t es;
    uint64_t ds;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
  };
  struct ArchThrd {
    uint64_t cr3;
    uint64_t gs;
    uint64_t fs;
  };

uint64_t *push(uint64_t *stack, uint64_t value);

registers setupKernelRegisters(const uint64_t start, const uint64_t stack, const uint64_t arg1);

uint64_t setupTask(Thread &thread, uint64_t *stack, const uint64_t size, const registers &regs);

void registerDump(const registers &regs);
}
