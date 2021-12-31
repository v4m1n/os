#pragma once
#include "stddef.h"

struct spinlock {
  void lock();
  void unlock();
  bool try_lock();

private:
  volatile size_t locked_ = 0;
};

struct spinlock_irq {
  void lock();
  void unlock();
  bool try_lock();

private:
  volatile size_t locked_ = 0;
  bool if_;
};
