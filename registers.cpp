module;
#include <cstdint>
#include <cstddef>
#include <atomic>

module registers;
import cpu;
import knew;
import scheduler;
import string;
import kmm;
import debug;
import vmm;
import interrupts;
import msr;

namespace thrd {

uint64_t *push(uint64_t *stack, uint64_t value) {
  --stack;
  *stack = value;
  return stack;
}

registers setupRegisters(const uint64_t start, const uint64_t stack, const uint64_t arg1, const uint64_t cs, const uint64_t ds) {
  registers regs;
  memset(&regs, 0, sizeof(regs));
  regs.rflags = 0x202;
  regs.rip = start;
  regs.rsp = stack;
  regs.rdi = arg1;
  regs.cs = cs;
  regs.ds = ds;
  regs.ss = ds;
  regs.es = ds;
  regs.fs_base = 0;
  regs.gs_base = rdmsr(IA32_GS_BASE_MSR);
  return regs;
}

void switchToKernelAddressSpace() {
  auto cur = getCPUStorage<CPU>(0)->current_thread_;
  std::atomic_ref cr3(cur->arch_.cr3);
  cr3 = vmm::AddressSpace::kernel_page_table_;
}

uint64_t setupTask(Thread &thread, uint64_t *stack, const uint64_t size, const registers &regs, const bool kernel) {

  uint8_t *st = reinterpret_cast<uint8_t *>(stack);
  st += (size - sizeof(registers));
  memcpy(st, &regs, sizeof(registers));
  stack = (uint64_t*)st;
  stack = push(stack, (uint64_t)context_return);
  for (size_t i = 0; i < 6; ++i)
    stack = push(stack, 0);
  if (kernel)
    thread.arch_.cr3 = vmm::AddressSpace::kernel_page_table_;
  else {
    thread.address_space_ = new vmm::AddressSpace;
    thread.arch_.cr3 = thread.address_space_->pml4_;
  }

  return reinterpret_cast<uint64_t>(stack);
}

void registerDump(const registers &regs) {
  dbg::printf("rax: {}, rbx: {}, rcx: {}\n"
              "rdx: {}, rsi: {}, rdi: {}\n"
              "rbp: {}, r8:  {}, r9:  {}\n"
              "r10: {}, r11: {}, r12: {}\n"
              "r13: {}, r14: {}, r15: {}\n"
              "cs:  {}, ds:  {}, es:  {}\n"
              "ss:  {}, fs:  {}, gs:  {}\n"
              "rip: {}, rsp: {}, rfl: {}\n",
              regs.rax, regs.rbx, regs.rcx, regs.rdx,
              regs.rsi, regs.rdi, regs.rbp, regs.r8,
              regs.r9, regs.r10, regs.r11, regs.r12,
              regs.r13, regs.r14, regs.r15, regs.cs,
              regs.ds, regs.es, regs.ss, regs.fs_base,
              regs.gs_base, regs.rip, regs.rsp, regs.rflags);
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
