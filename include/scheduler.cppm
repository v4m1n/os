module;
#include <cstdint>
#include <cstddef>

export module scheduler;
import vmm;
import registers;
import gdt;
import sync;
import interrupts;
import array;

export extern "C++" struct Thread {
  uint64_t stack_[1024];
  bool sleeping_;
  uint64_t current_stack_;
  Thread *next_;
  Thread *prev_;
  thrd::ArchThrd arch_;
  vmm::AddressSpace *address_space_ = nullptr;
};

export struct CPU {
  constexpr CPU() = default;
  struct CPU *cpu_ = this;
  uint64_t scratch_;
  Thread *current_thread_;
  uint8_t id_;
  size_t stack_start_;
  size_t tick_ = 0;
  ArchCPU arch_;
  Thread *cleanup_list_ = nullptr;
  spinlock_irq list_lock_;
  Thread *list_ = nullptr;
};

export extern array<CPU> cpus;

export namespace sched {

  void schedule();

  void launch();

  void idle();

  void addThread(Thread *thread);

  Thread *nextThread();

  Thread *popThread();

  [[nodiscard]] Thread *createKernelThread(size_t function, size_t arg);

  [[nodiscard]] Thread *createUserThread(size_t function, size_t arg);

}
