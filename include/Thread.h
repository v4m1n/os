#pragma once
#include "stdint.h"


struct Thread {
  uint64_t stack_[1024];
  bool sleeping_;
  uint64_t current_stack_;
  Thread *next_;
  Thread *prev_;
};

extern Thread *currentThread;
