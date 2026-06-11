.intel_syntax noprefix
.section .text.prologue
.global _start
_start:
  xor rbp, rbp
  call main
  mov rdi, rax
  call exit
1:  jmp 1b
