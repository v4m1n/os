#pragma once
#include "stdint.h"
#include "stddef.h"

namespace kmm {
  void testslab();
  void *kmalloc(size_t size);
  void kfree(void *ptr);
}
