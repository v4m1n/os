module;

#include <algorithm>
#include <cstdint>

module syscall;

import debug;

namespace syscall {
  uint64_t handler(uint64_t snum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    dbg::printf("syscall {} with parameters {} {} {} {} {} {}\n", snum, arg1, arg2, arg3, arg4, arg5, arg6);
    
    switch (snum) {
      case 1:
        {
          dbg::printf("[USER] ");
          char *out = (char*)arg2;
          for (uint64_t i = 0; i < arg3; ++i) {
            dbg::putchar(out[i]);
          }
        }
        break;
      default:
        dbg::printf("unknown syscall number {}\n", snum);
        break;
    }
    return -1;
  }
}
