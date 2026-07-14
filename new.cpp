#include <cstddef>
#include <cstdint>
import knew;

import kmm;
import debug;


void *operator new(size_t size) {
  auto mem = kmm::kmalloc(size);
  dbg::panic_assert(mem, "new failed");
  return mem;
}
void *operator new[](size_t size) {
  auto mem = kmm::kmalloc(size);
  dbg::panic_assert(mem, "new failed");
  return mem;
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


namespace std {
[[noreturn]] void __glibcxx_assert_fail(char const* file, int line, char const* function, char const* condition) {
  dbg::panic("libstdc++ assert failed: {} at {}:{} in {}\n", condition, file, (uint64_t)line, function);
  while (true) {}
}
}


