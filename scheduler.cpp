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

void launch() {
  addThread(createKernelThread(reinterpret_cast<size_t>(idle), 0));

  uint64_t tmp;
  auto cpu = getCPUStorage<CPU>(0);
  cpu->current_thread_ = getCPUStorage<CPU>(0)->list_;
  context_switch(&cpu->current_thread_->current_stack_, &tmp);
  dbg::panic("end of scheduler launch function reached\n");
}

[[nodiscard]] Thread *createKernelThread(size_t function, size_t arg) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));
  const auto regs = thrd::setupRegisters(function, reinterpret_cast<size_t>(thread->stack_)+sizeof(thread->stack_), arg);
  thread->current_stack_ = thrd::setupTask(*thread, thread->stack_, sizeof(thread->stack_), regs);
  thread->pid_ = ++pid_cnt;
  return thread;
}

extern "C" uint8_t test_code_start;
extern "C" uint8_t test_code_end;

asm(R"(
.global test_code_start
.global test_code_end
test_code_start:
mov %rax, %rdi
mov %rbx, 1
int 0x80
mov %rcx, 0xdeadbeef
int 0x80
mov %rdx, 0x0a414141414141
int 0x80

push %rdx
mov %rax, 1
mov %rdi, 0
mov %rsi, %rsp
mov %rcx, 8
int 0x80

1: jmp 1b


test_code_end:
)");

[[nodiscard]] Thread *createUserThread(size_t function, size_t arg, util::shared_ptr<Loader> &loader) {
  auto thread = reinterpret_cast<Thread *>(kmm::kmalloc(sizeof(Thread)));
  memset(thread, 0, sizeof(Thread));

  //constexpr uint64_t CODE_START = 0x8000000ULL;
  constexpr uint64_t STACK_START = (1ULL<<47)-PAGE_SIZE*1024;

  const auto regs = thrd::setupRegisters(function, STACK_START+PAGE_SIZE, arg, USER_CS, USER_DS);
  thread->current_stack_ = thrd::setupTask(*thread, thread->stack_, sizeof(thread->stack_), regs);
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
  while(1) {
    if (!getCPUStorage<CPU>(0)->cleanup_list_) {
      schedule();
      continue;
    }
    irq::disableInterrupts();
    Thread *cleanup_list_ = getCPUStorage<CPU>(0)->cleanup_list_;
    getCPUStorage<CPU>(0)->cleanup_list_ = 0;
    irq::enableInterrupts();
    (void) cleanup_list_;

  }
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
