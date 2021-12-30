#include "registers.h"
#include "interrupts.h"
#include "gdt.h"
#include "stddef.h"
#include "string.h"

namespace thrd {

uint64_t *push(uint64_t *stack, uint64_t value) {
  --stack;
  *stack = value;
  return stack;
}

registers setupKernelRegisters(const uint64_t start, const uint64_t stack, const uint64_t arg1) {
  registers regs;
  memset(&regs, 0, sizeof(regs));
  regs.rflags = 0x202;
  regs.rip = start;
  regs.rsp = stack;
  regs.rdi = arg1;
  regs.cs = KERNEL_CS;
  regs.ds = KERNEL_DS;
  regs.ss = KERNEL_DS;
  regs.es = KERNEL_DS;
  return regs;
}

uint64_t setupTask(uint64_t *stack, const uint64_t size, const registers &regs) {

  uint8_t *st = reinterpret_cast<uint8_t *>(stack);
  st += (size - sizeof(registers));
  memcpy(st, &regs, sizeof(registers));
  stack = (uint64_t*)st;
  stack = push(stack, (uint64_t)context_return);
  for (size_t i = 0; i < 6; ++i)
    stack = push(stack, 0);
  return reinterpret_cast<uint64_t>(stack);

}

}
