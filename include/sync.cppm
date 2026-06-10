module;
#include <cstddef>

export module sync;

export struct spinlock {
  void lock();
  void unlock();
  bool try_lock();

  spinlock() = default;
  spinlock(const spinlock &) = delete;
  spinlock(const spinlock &&) = delete;
  spinlock &operator=(const spinlock &) = delete;
  spinlock &operator=(const spinlock &&) = delete;

private:
  volatile size_t locked_ = 0;
};

export struct spinlock_irq {
  void lock();
  void unlock();
  bool try_lock();

  spinlock_irq() = default;
  spinlock_irq(const spinlock_irq &) = delete;
  spinlock_irq(const spinlock_irq &&) = delete;
  spinlock_irq &operator=(const spinlock_irq &) = delete;
  spinlock_irq &operator=(const spinlock_irq &&) = delete;
private:
  volatile size_t locked_ = 0;
  bool if_ = false;
};

export using mutex = spinlock;

export template <typename T>
class lock_guard {
  bool release_;
  T &lock_;

public:
  lock_guard(const lock_guard &) = delete;
  lock_guard(const lock_guard &&) = delete;
  lock_guard &operator=(const lock_guard &) = delete;
  lock_guard &operator=(const lock_guard &&) = delete;

  lock_guard(T &lock) : release_(true), lock_(lock){
    lock_.lock();
  }
  void release() {
    lock_.unlock();
    release_ = false;
  }
  ~lock_guard() {
    if (release_)
      lock_.unlock();
  }
};
