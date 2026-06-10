module;
#include "stddef.h"

export module knew;

export extern "C++" {
  void *operator new(size_t, void *p);
  void *operator new[](size_t, void *p);
  void  operator delete  (void *, void *);
  void  operator delete[](void *, void *);

  void *operator new(size_t size);
  void *operator new[](size_t size);
  void operator delete(void *p) noexcept;
  void operator delete[](void *p) noexcept;
  void operator delete(void *p, size_t size) noexcept;
  void operator delete[](void *p, size_t size) noexcept;
}
