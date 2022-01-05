#pragma once
#include "Thread.h"
#include "stddef.h"
#include "array.h"
#include "interrupts.h"
#include "sync.h"

struct CPU {
  struct CPU *cpu_ = this;
  uint8_t id_;
  size_t stack_start_;
  ArchCPU arch_;
  spinlock_irq list_lock_;
  Thread *list_ = nullptr;
};

extern array<CPU> cpus;

namespace sched {

  void schedule();

  void launch();

  void addThread(Thread *thread);

  Thread *nextThread();

  Thread *popThread();

  [[nodiscard]] Thread *createKernelThread(size_t function, size_t arg);

}
