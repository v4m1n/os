module;


#include <cstdlib>
#include <cstdint>


export module syscall;
export namespace syscall {

  uint64_t handler(uint64_t snum, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);


}
