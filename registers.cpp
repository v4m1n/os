#include "registers.h"
#include "interrupts.h"
#include "gdt.h"
#include "stddef.h"
#include "string.h"
#include "scheduler.h"
#include "Thread.h"

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

uint64_t setupTask(Thread &thread, uint64_t *stack, const uint64_t size, const registers &regs) {

  uint8_t *st = reinterpret_cast<uint8_t *>(stack);
  st += (size - sizeof(registers));
  memcpy(st, &regs, sizeof(registers));
  stack = (uint64_t*)st;
  stack = push(stack, (uint64_t)context_return);
  for (size_t i = 0; i < 6; ++i)
    stack = push(stack, 0);
  //thread->arch_.cr3 = vmm::AddressSpace::kernel_page_table_;
  thread.address_space_ = reinterpret_cast<vmm::AddressSpace *>(kmm::kmalloc(sizeof(vmm::AddressSpace)));
  new (thread.address_space_) vmm::AddressSpace;
  thread.arch_.cr3 = thread.address_space_->pml4_;

  return reinterpret_cast<uint64_t>(stack);
}

extern "C"
void taskSwitch() {
  auto cpu = getCPUStorage<CPU>(0);
  auto currentThread = cpu->current_thread_;
  auto stack = reinterpret_cast<uint64_t>(currentThread->stack_) + sizeof(currentThread->stack_);
  cpu->arch_.tss.rsp0_low = stack;
  cpu->arch_.tss.rsp0_high = stack>>32;
  setCR3(currentThread->arch_.cr3);
}

}
