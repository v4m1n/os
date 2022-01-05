#pragma once
#include "stdint.h"
#include "vmm.h"
#include "registers.h"

struct Thread {
  uint64_t stack_[1024];
  bool sleeping_;
  uint64_t current_stack_;
  Thread *next_;
  Thread *prev_;
  thrd::ArchThrd arch_;
  vmm::AddressSpace *address_space_ = nullptr;
};
