.intel_syntax noprefix
.code32
.global _start
.section .text

kernel_pg_addr:
.global gdt

_start: cli
mov al, 0x41
mov edi, ebx

mov eax, cr4
or eax, (1<<5)|(1<<9)
mov cr4, eax
mov ecx, 0xC0000080
rdmsr
or eax, (1<<11)|(1<<8)
wrmsr
//this is horrible, but better than fucking around with the linker
.code64 
lea rax, qword [pml4]
lea rbx, qword [pdpt]
lea rcx, qword [pd]
lea rdx, [gdt]
lea rsi, [tmp]
.code32
and eax, 0xffff000U
and ebx, 0xffff000U
and ecx, 0xffff000U
and edx, 0xfffffffU
and esi, 0xfffffffU
or eax, 0x3U
or ebx, 0x3U
or ecx, 0x3U
mov dword ptr [eax-3], ebx
mov dword ptr [eax+1], 0
mov dword ptr [eax-3+8*511], ebx
mov dword ptr [eax+1+8*511], 0
mov dword ptr [ebx-3], ecx
mov dword ptr [ebx+1], 0
mov dword ptr [ebx-3+8*510], ecx
mov dword ptr [ebx+1+8*510], 0
and ecx, 0xffff000U
mov ebx, 0x83
map: 
mov dword ptr [ecx], ebx
mov dword ptr [ecx+4], 0
add ebx, (1<<21)
add ecx, 8
cmp ebx, (1<<30)
jbe map
and eax, 0xffff000U
mov cr3, eax
push 0
push edx
push ((4*16-1)<<16)
lea eax, [esp+2]
lgdt [eax]
mov eax, cr0
or eax, (1<<31)
mov cr0, eax
pushf
push 0x8
push esi
iret
.code64
tmp:
and rsp, ~0xfULL
push 0x10
push rsp
pushf
push 0x8
lea rax, [lmode]
push rax
iretq
lmode:
mov ax, 0x10
mov ss, ax
mov ds, ax
mov fs, ax
mov gs, ax
mov es, ax
//mov qword ptr [pml4], 0
//mov qword ptr [pdpt], 0
lea rax, [gdt]
mov word ptr [rsp], 0x60-1
mov [rsp+2], rax
lgdt [rsp]
lea rsp, [boot_stack+4096*2]
jmp boot
label: jmp label
