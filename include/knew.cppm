module;
#include <cstddef>

export module knew;

export extern "C++" {
  constexpr void *operator new(size_t, void *p) noexcept { return p; }
  constexpr void *operator new[](size_t, void *p) noexcept { return p; }
  constexpr void  operator delete  (void *, void *) noexcept {}
  constexpr void  operator delete[](void *, void *) noexcept {}

  void *operator new(size_t size);
  void *operator new[](size_t size);
  void operator delete(void *p) noexcept;
  void operator delete[](void *p) noexcept;
  void operator delete(void *p, size_t size) noexcept;
  void operator delete[](void *p, size_t size) noexcept;
}
