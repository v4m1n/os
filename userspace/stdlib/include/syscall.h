#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t syscall0(uint64_t num);
uint64_t syscall1(uint64_t num, uint64_t a1);
uint64_t syscall2(uint64_t num, uint64_t a1, uint64_t a2);
uint64_t syscall3(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3);

#ifdef __cplusplus
}
#endif
