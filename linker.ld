ENTRY(_start)
LS_Virt = 0xFFFFFFFF80000000;
LS_Phys = 0x100000;
SECTIONS 
{
    . = 0xFFFFFFFF80000000+0x100000;

    .text : AT(ADDR(.text)-0xFFFFFFFF80000000)
    {
        *(.multiboot)
        *(.text)
    }

    .bss : AT(ADDR(.bss)-0xFFFFFFFF80000000)
    {
        *(.bss)
    }
}
