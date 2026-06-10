module;
#include <cstdint>
#include <cstddef>

export module kmm;

export namespace kmm {
  void testslab();
  void *kmalloc(size_t size);
  void kfree(void *ptr);
  void kmalloc_init();
}
