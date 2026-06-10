#include <cstddef>
#include <cstdint>
import knew;

import kmm;


void *operator new(size_t size) {
  return kmm::kmalloc(size);
}
void *operator new[](size_t size) {
  return kmm::kmalloc(size);
}
void operator delete(void *p) noexcept {
  kmm::kfree(p);
}
void operator delete[](void *p) noexcept {
  kmm::kfree(p);
}
void operator delete(void *p, size_t) noexcept {
  kmm::kfree(p);
}
void operator delete[](void *p, size_t) noexcept {
  kmm::kfree(p);
}

import debug;

namespace std {
[[noreturn]] void __glibcxx_assert_fail(char const* file, int line, char const* function, char const* condition) {
  dbg::panic("libstdc++ assert failed: {} at {}:{} in {}\n", condition, file, (uint64_t)line, function);
  while (true) {}
}
}


