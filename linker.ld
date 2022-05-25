ENTRY(_start)
LS_Virt = 0xFFFFFFFF80000000;
LS_Phys = 0x100000;
SECTIONS 
{
    . = LS_Virt+LS_Phys;
    kernel_start = .;

    .text ALIGN(4096) : AT(ADDR(.text)-LS_Virt)
    {
        *(.multiboot)
        *(.text)
    }
    .data ALIGN(4096) : AT(ADDR(.data)-LS_Virt)
    {
        *(.data)
    }

    .rodata ALIGN(4096) : AT(ADDR(.rodata)-LS_Virt)
    {
        *(.rodata)
    }
    .bss ALIGN(4096) : AT(ADDR(.bss)-LS_Virt)
    {
        bss_start = .;
        *(.bss)
        bss_end = .;

    }
    kernel_end = .;
}
