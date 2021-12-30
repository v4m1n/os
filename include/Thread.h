#pragma once
#include "stdint.h"

namespace thrd {
struct Thread {
  uint64_t stack_[1024];
  bool sleeping_;
  uint64_t current_stack;
};
}
