module;
#include <cstdint>
#include <cstddef>
#include <atomic>

module scheduler;
import cpu;
import string;
import kmm;
import debug;
import registers;
import interrupts;
import pmm;
import array;

array<CPU> cpus(0);

namespace sched {

std::atomic<uint64_t> pid_cnt;
static_assert(decltype(pid_cnt)::is_always_lock_free);

Thread *getCurrentThread() {
  auto cpu = getCPUStorage<CPU>(0);
  return cpu->current_thread_;
}

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


[[nodiscard]] Thread *createKernelThread(size_t function, size_t arg) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));
  const auto regs = thrd::setupRegisters(function, reinterpret_cast<size_t>(thread->stack_)+sizeof(thread->stack_), arg);
  thread->current_stack_ = thrd::setupTask(*thread, thread->stack_, sizeof(thread->stack_), regs, true);
  thread->pid_ = ++pid_cnt;
  return thread;
}

[[noreturn]] void suicide(uint64_t exit_code) {
  (void)exit_code;
  irq::disableInterrupts();
  auto cpu = getCPUStorage<CPU>(0);
  auto cur = cpu->current_thread_;

  cur->loader_ = nullptr;
  thrd::switchToKernelAddressSpace();
  cur->address_space_ = nullptr;

  cpu->list_lock_.lock(); // remove thread from the list
  cur->next_->prev_ = cur->prev_;
  cur->prev_->next_ = cur->next_;
  cpu->list_ = cur->prev_;
  cpu->list_lock_.unlock();

  cpu->cleanup_lock_.lock(); // add for cleanup
  cur ->next_ = cpu->cleanup_list_;
  cpu->cleanup_list_ = cur;
  cpu->cleanup_lock_.unlock();


  schedule(); // de-schedule


  dbg::panic("thread survived suicide\n");

}


[[nodiscard]] Thread *createUserThread(size_t function, size_t arg, util::shared_ptr<Loader> &loader) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));

  //constexpr uint64_t CODE_START = 0x8000000ULL;
  constexpr uint64_t STACK_START = (1ULL<<47)-PAGE_SIZE*1024;

  const auto regs = thrd::setupRegisters(function, STACK_START+PAGE_SIZE, arg, USER_CS, USER_DS);
  thread->current_stack_ = thrd::setupTask(*thread, thread->stack_, sizeof(thread->stack_), regs, false);
  //auto code_page = pmm::allocZeroPFN();
  //auto code = vmm::pageAddress<void *>(code_page);
  //memcpy(code, &test_code_start, ((uint64_t)&test_code_end-(uint64_t)&test_code_start));
  //thread->address_space_->mapPFN(CODE_START/PAGE_SIZE, code_page, 0, 0);
  //thread->address_space_->mapPFN(STACK_START/PAGE_SIZE, stack_page, 1, 0);
  thread->pid_ = ++pid_cnt;
  thread->loader_ = loader;

  return thread;
}

void cleanupTask() {
  auto cpu = getCPUStorage<CPU>(0);
  while(1) {
    if (!cpu->cleanup_list_) {
      schedule();
      continue;
    }
    cpu->cleanup_lock_.lock();
    Thread *cleanup_list = cpu->cleanup_list_;
    getCPUStorage<CPU>(0)->cleanup_list_ = 0;
    cpu->cleanup_lock_.unlock();

    size_t cnt = 0;

    while (cleanup_list) {
      auto old = cleanup_list;
      cleanup_list = cleanup_list->next_;
      delete old;
      ++cnt;
    }
    dbg::printf("cleanup done, deleted {} threads\n", cnt);
  }
}

void idleTask() {
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

void launch() {
  addThread(createKernelThread(reinterpret_cast<size_t>(idleTask), 0));
  addThread(createKernelThread(reinterpret_cast<size_t>(cleanupTask), 0));

  uint64_t tmp;
  auto cpu = getCPUStorage<CPU>(0);
  cpu->current_thread_ = getCPUStorage<CPU>(0)->list_;
  context_switch(&cpu->current_thread_->current_stack_, &tmp);
  dbg::panic("end of scheduler launch function reached\n");
}
}
