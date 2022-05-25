.intel_syntax noprefix
.code32
.global _start
.section .text

#define OFFSET 0xFFFFFFFF80000000ULL

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

lea eax, [pml4-OFFSET]
lea ebx, [pdpt-OFFSET]
lea ecx, [pd-OFFSET]
lea edx, [gdt-OFFSET]
lea esi, [tmp-OFFSET]
mov dword ptr [eax], ebx
mov dword ptr [eax+4], 0
or dword ptr [eax], 3
mov dword ptr [eax+8*511], ebx
mov dword ptr [eax+4+8*511], 0
or dword ptr [eax+8*511], 3
mov dword ptr [ebx], ecx
mov dword ptr [ebx+4], 0
or dword ptr [ebx], 3
mov dword ptr [ebx+8*510], ecx
mov dword ptr [ebx+4+8*510], 0
or dword ptr [ebx+8*510], 3
mov ebx, 0x83
map: 
mov dword ptr [ecx], ebx
mov dword ptr [ecx+4], 0
add ebx, (1<<21)
add ecx, 8
cmp ebx, (1<<30)
jbe map

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
lea rax, [gdt]
mov word ptr [rsp], 0x60-1
mov [rsp+2], rax
lgdt [rsp]
lea rsp, [boot_stack+4096*2]
jmp boot
