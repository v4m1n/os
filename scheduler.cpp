#include "scheduler.h"
#include "interrupts.h"
#include "debug.h"
#include "Thread.h"
#include "string.h"
#include "kmm.h"
#include "registers.h"
#include "asm.h"

array<CPU> cpus(0);

namespace sched {

void schedule() {
  bool IF = irq::getIF();
  irq::disableInterrupts();
  auto cpu = getCPUStorage<CPU>(0);

  auto old = cpu->current_thread_;
  cpu->current_thread_ = nextThread();
  context_switch(&cpu->current_thread_->current_stack_, &old->current_stack_);

  if (IF)
    irq::enableInterrupts();
}

void addThread(Thread *thread) {
  auto cpu = getCPUStorage<CPU>(0);
  lock_guard lock(cpu->list_lock_);
  auto &list = cpu->list_;

  if (list == nullptr) {
    list = thread;
    thread->next_ = thread;
    thread->prev_ = thread;
    return;
  }
  thread->next_ = list->next_;
  thread->prev_ = list;
  list->next_->prev_ = thread;
  list->next_ = thread;
}

Thread *nextThread() {
  auto cpu = getCPUStorage<CPU>(0);
  lock_guard lock(cpu->list_lock_);
  auto &list = cpu->list_;
  do {
    list = list->next_;
  } while (list->sleeping_);
  return list;
}

Thread *popThread() {
  auto cpu = getCPUStorage<CPU>(0);
  lock_guard lock(cpu->list_lock_);
  auto &list = cpu->list_;
  auto thread = list;
  list = list->prev_;
  list->next_ = thread->next_;
  list->next_->prev_ = list;
  return thread;
}

void launch() {
  addThread(createKernelThread(reinterpret_cast<size_t>(idle), 0));

  uint64_t tmp;
  auto cpu = getCPUStorage<CPU>(0);
  cpu->current_thread_ = getCPUStorage<CPU>(0)->list_;
  dbg::printf("crr {}\n", cpu->current_thread_);
  context_switch(&cpu->current_thread_->current_stack_, &tmp);
  dbg::panic("end of scheduler launch function reached\n");
}

[[nodiscard]] Thread *createKernelThread(size_t function, size_t arg) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));
  const auto regs = thrd::setupKernelRegisters(function, reinterpret_cast<size_t>(thread->stack_)+sizeof(thread->stack_), arg);
  thread->current_stack_ = thrd::setupTask(*thread, thread->stack_, sizeof(thread->stack_), regs);
  return thread;
}
void idle() {
  size_t tick = getCPUStorage<CPU>(0)->tick_;
  schedule();
  while(1) {
    if (tick == getCPUStorage<CPU>(0)->tick_) {
      hlt();
      ++tick;
    }
    else {
      tick = getCPUStorage<CPU>(0)->tick_;
      schedule();
    }
  }
}
}
