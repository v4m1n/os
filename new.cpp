#include "stddef.h"
import knew;

import kmm;

void *operator new(size_t, void *p) { return p; }
void *operator new[](size_t, void *p) { return p; }
void  operator delete  (void *, void *) { };
void  operator delete[](void *, void *) { };

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
