#pragma once
#include "Thread.h"
#include "stddef.h"
#include "array.h"

struct CPU {
  uint8_t id_;
  size_t stack_start_;
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
