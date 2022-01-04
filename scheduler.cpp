#include "scheduler.h"
#include "interrupts.h"
#include "debug.h"
#include "Thread.h"
#include "string.h"
#include "kmm.h"
#include "registers.h"

Thread *currentThread = nullptr;

array<CPU> cpus(0);

namespace sched {

static Thread *list = nullptr;

void schedule() {
  bool IF = irq::getIF();
  irq::disableInterrupts();

  auto old = currentThread;
  currentThread = nextThread();
  context_switch(&currentThread->current_stack_, &old->current_stack_);

  if (IF)
    irq::enableInterrupts();
}

void addThread(Thread *thread) {
  bool IF = irq::getIF();
  irq::disableInterrupts();

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
  if (IF)
    irq::enableInterrupts();
}

Thread *nextThread() {
  do {
    list = list->next_;
  } while (list->sleeping_);
  return list;
}

Thread *popThread() {
  auto thread = list;
  list = list->prev_;
  list->next_ = thread->next_;
  list->next_->prev_ = list;
  return thread;
}

void launch() {
  uint64_t tmp;
  currentThread = list;
  context_switch(&currentThread->current_stack_, &tmp);
  dbg::panic("end of scheduler launch function reached\n");
}

[[nodiscard]] Thread *createKernelThread(size_t function, size_t arg) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));
  const auto regs = thrd::setupKernelRegisters(function, reinterpret_cast<size_t>(thread->stack_)+sizeof(thread->stack_), arg);
  thread->current_stack_ = thrd::setupTask(thread->stack_, sizeof(thread->stack_), regs);
  return thread;
}
}
