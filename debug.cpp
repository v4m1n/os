#include "stdint.h"
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


}
