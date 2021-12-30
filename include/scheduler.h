#pragma once
#include "Thread.h"
#include "stddef.h"

namespace sched {

  void schedule();

  void launch();

  void addThread(Thread *thread);

  Thread *nextThread();

  Thread *popThread();

  [[nodiscard]] Thread *createKernelThread(size_t function, size_t arg);

}
