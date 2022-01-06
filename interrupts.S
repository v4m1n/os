.intel_syntax noprefix

#define KERNEL_DS 0x20
#define IA32_FS_BASE_MSR 0xc0000100
#define IA32_GS_BASE_MSR 0xc0000101
#define IA32_KERNEL_GS_BASE_MSR 0xc0000102

.macro popall
  mov eax, [rsp]
  mov edx, [rsp+4]
  mov ecx, IA32_GS_BASE_MSR
  wrmsr
  mov eax, [rsp+8]
  mov edx, [rsp+12]
  mov ecx, IA32_FS_BASE_MSR
  wrmsr
  add rsp, 16
  pop rax
  mov es, ax
  pop rax
  mov ds, ax
  pop rax
  pop rbx
  pop rcx
  pop rdx
  pop rsi
  pop rdi
  pop rbp
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15
.endm

.macro pushall
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rbp
  push rdi
  push rsi
  push rdx
  push rcx
  push rbx
  push rax
  mov ax, ds
  push rax
  mov ax, es
  push rax
  mov ax, KERNEL_DS
  mov ds, ax
  mov es, ax
  mov ecx, IA32_FS_BASE_MSR
  rdmsr
  mov [rsp-4], edx
  mov [rsp-8], eax
  mov ecx, IA32_KERNEL_GS_BASE_MSR
  rdmsr
  mov [rsp-12], edx
  mov [rsp-16], eax
  sub rsp, 16
.endm

.macro irq n
.global asm_irq_handler_\n
asm_irq_handler_\n:
  pushall
  mov rdi, rsp
  call irq_handler_\n
  popall
  iretq
.endm

.macro exception n
.global asm_exception_handler_\n
asm_exception_handler_\n:
  pushall
  mov rdi, rsp
  call exception_handler_\n
  popall
  iretq
.endm

.macro exceptionerror n
.global asm_exception_handler_\n
asm_exception_handler_\n:
  push rax
  mov rax, [rsp + 8]
  swapgs
  mov [gs:8], rax
  pop rax
  add rsp, 8
  pushall
  mov rdi, rsp
  mov rsi, [gs:8]
  swapgs
  call exception_handler_\n
  popall
  iretq
.endm



.irp n,0
exception \n
.endr

.irp n,8,13,14
exceptionerror \n
.endr

.irp n,48,128,254,255,def
irq \n
.endr


.global context_switch
context_switch:
push rbx
push rbp
push r12
push r13
push r14
push r15
mov [rsi], rsp
mov rsp, [rdi]
call taskSwitch
pop r15
pop r14
pop r13
pop r12
pop rbp
pop rbx
ret

.global context_return
context_return:
  popall
  iretq
