#include <cstddef>
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
