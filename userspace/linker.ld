ENTRY(_start)

SECTIONS
{
  . = 0x8000000;

  .text : {
    *(.text.prologue)
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
  }

  /DISCARD/ : {
    *(.comment)
    *(.eh_frame)
    *(.note*)
  }
}
