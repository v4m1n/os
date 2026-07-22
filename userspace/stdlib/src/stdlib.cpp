#include "stdlib.h"
#include "syscall.h"

extern "C" void exit(int status) {
  syscall1(60, (uint64_t)status); // syscall 0 for exit
  while (1) {
    // Halt/loop if exit didn't terminate the thread
  }
}
