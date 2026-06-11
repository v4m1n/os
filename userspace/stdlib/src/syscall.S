.intel_syntax noprefix
.section .text

.global syscall0
syscall0:
  mov rax, rdi
  int 0x80
  ret

.global syscall1
syscall1:
  mov rax, rdi
  mov rdi, rsi
  int 0x80
  ret

.global syscall2
syscall2:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  int 0x80
  ret

.global syscall3
syscall3:
  mov rax, rdi
  mov rdi, rsi
  mov rsi, rdx
  mov rdx, rcx
  int 0x80
  ret
