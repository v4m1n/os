#include <stdint.h>

extern "C" void __cxa_pure_virtual() {
  while (1);
}

void* operator new(size_t size) {
  (void)size;
  return nullptr;
}

void operator delete(void* ptr) noexcept {
  (void)ptr;
}

void operator delete(void* ptr, size_t size) noexcept {
  (void)ptr;
  (void)size;
}
