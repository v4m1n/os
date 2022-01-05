#pragma once
#include "Thread.h"
#include "stddef.h"
#include "array.h"
#include "interrupts.h"
#include "sync.h"

struct CPU {
  constexpr CPU() = default;
  struct CPU *cpu_ = this;
  uint64_t scratch_;
  Thread *current_thread_;
  uint8_t id_;
  size_t stack_start_;
  size_t tick_ = 0;
  ArchCPU arch_;
  spinlock_irq list_lock_;
  Thread *list_ = nullptr;
};

extern array<CPU> cpus;

namespace sched {

  void schedule();

  void launch();

  void idle();

  void addThread(Thread *thread);

  Thread *nextThread();

  Thread *popThread();

  [[nodiscard]] Thread *createKernelThread(size_t function, size_t arg);

}
