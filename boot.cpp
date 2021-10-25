#include "stdint.h"
#include "debug.h"
#include "multiboot.h"
#include "new.h"
#include "PageManager.h"

[[maybe_unused]] const struct
{
    unsigned int magic = 0xe85250d6;
    unsigned int flags = 0;
    unsigned int size = 32*6;
    unsigned int checksum = -(0xe85250d6U+32U*6U);
    unsigned int end_tag = 0;
    unsigned int end_tag2 = 8;
} multiboot __attribute__ ((section (".multiboot")));

[[gnu::aligned(4096)]] uint64_t pml4[512];
[[gnu::aligned(4096)]] uint64_t pdpt[512];
[[gnu::aligned(4096)]] uint64_t pd[512];

uint64_t boot_stack[512];


struct GDTE {
  uint16_t limit1;
  uint16_t base1;
  uint8_t base2;
  uint8_t accessed : 1;
  uint8_t rw : 1;
  uint8_t direction : 1;
  uint8_t executable : 1;
  uint8_t descriptor : 1;
  uint8_t priv : 2;
  uint8_t present : 1;
  uint8_t limit2 : 4;
  uint8_t zero : 1;
  uint8_t lmode : 1;
  uint8_t size : 1;
  uint8_t granularity : 1;
  uint8_t base3;
} __attribute__((packed));
static_assert(sizeof(GDTE) == 8);

#define KERNEL_CS 0x8
#define KERNEL_DS 0x10
#define TSS 0x18
#define USER_CS 0x23
#define USER_DS 0x2b

extern void *LS_Virt[];
extern void *bss_end[];
const uint64_t VIRTUAL_OFFSET = (uint64_t)LS_Virt;

GDTE gdt[6] = {{},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0},
               {.limit1=0x68U, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=0, .priv=0,
                .present=1, .limit2=0, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0}};

extern "C"
[[noreturn]] void boot(uint64_t mbootheader) {
  mbootheader += VIRTUAL_OFFSET;
  dbg::printf("booting...\n");
  dbg::printf("multiboot2 header at {}\n", (uint64_t *)mbootheader);

  new (&pm::instance) pm::PageManager;

  mboot::parse(mbootheader);
  dbg::printf("bss end at {}\n", (uint64_t *)bss_end);
  while(1);
}
