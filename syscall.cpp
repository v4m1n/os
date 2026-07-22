module;

#include <algorithm>
#include <cstdint>

module syscall;

import debug;
import scheduler;
import kmm;
import vmm;

namespace syscall {
  uint64_t handler(uint64_t snum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    dbg::printf("syscall {} with parameters {} {} {} {} {} {}\n", snum, arg1, arg2, arg3, arg4, arg5, arg6);
    uint64_t ret = -1;
    
    switch (static_cast<Syscall>(snum)) {
      case SYS_WRITE:
        {

          char *out = (char *)kmm::kmalloc(arg3+1);
          if (!out) goto write_done;


          if (!vmm::copyFromUser(out, (void*)arg2, arg3)) goto write_done;
          out[arg3] = 0;

          dbg::printf("[USER] ");
          dbg::printf(out);
          dbg::putchar('\n');
          ret = arg3;
        write_done:
          kmm::kfree(out);
        }
        break;
      case SYS_SCHED_YIELD:
        {
          dbg::printf("yield\n");
          sched::schedule();
          ret = 0;
        }
        break;
      case SYS_EXIT:
        {
          dbg::printf("exit called\n");
          sched::suicide(arg1);
        }
        break;
      default:
        dbg::printf("unknown syscall number {}\n", snum);
        break;
    }
    return ret;
  }
}
