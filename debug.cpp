#include "stdint.h"
#include "stddef.h"
#include "debug.h"

namespace dbg {

  void printf(const char *str) {
    while(*str) {
      putchar(*str);
      ++str;
    }
  }
  void putchar(const char x) {
    asm volatile("out 0xe9, al"::"a"(x));
  }

  void dump(void *data, size_t size) {
    size = (size + 7) / 8;
    auto d = reinterpret_cast<uint64_t *>(data);
    for (size_t i = 0; i < size; ++i) {
      printf("{}", d[i]);
      if (i&1)
        putchar('\n');
      else
        putchar(' ');
    }

  }


}
