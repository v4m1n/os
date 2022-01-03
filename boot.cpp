#include "stdint.h"
#include "debug.h"
#include "multiboot.h"
#include "new.h"
#include "pmm.h"
#include "string.h"
#include "vmm.h"
#include "gdt.h"
#include "kmm.h"
#include "interrupts.h"
#include "registers.h"
#include "scheduler.h"
#include "Thread.h"
#include "sync.h"

[[maybe_unused]] const struct
{
    unsigned int magic = 0xe85250d6;
    unsigned int flags = 0;
    unsigned int size = 32*6;
    unsigned int checksum = -(0xe85250d6U+32U*6U);
    unsigned int end_tag = 0;
    unsigned int end_tag2 = 8;
} multiboot __attribute__ ((section (".multiboot")));


[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]]
uint8_t boot_stack[PAGE_SIZE*2];


extern void *LS_Virt[];
extern uint8_t *bss_end[];
extern uint8_t *bss_start[];
extern uint8_t *kernel_start[];
extern uint8_t *kernel_end[];
size_t VIRTUAL_OFFSET = (size_t)LS_Virt;
size_t KERNEL_START = (size_t)kernel_start;
size_t KERNEL_END = (size_t)kernel_end;

TSS tss;

void loadTSS() {
  tss.iopb_offset = sizeof(TSS);
  asm volatile("ltr ax"::"a"(0x30):"memory");
}

GDTE gdt[6] = {{},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0, .base4=0, .reserved=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0, .base4=0, .reserved=0},
               {.limit1=sizeof(TSS), .base1=(uint16_t)(uint64_t)&tss, .base2=(uint8_t)((uint64_t)&tss>>16), .accessed=1, .rw=0, .direction=0, .executable=1, .descriptor=0, .priv=0,
                .present=1, .limit2=0, .zero=0, .lmode=0, .size=1, .granularity=0, .base3=(uint8_t)((uint64_t)&tss>>24), .base4=(uint32_t)((uint64_t)&tss>>32), .reserved=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0, .base4=0, .reserved=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0, .base4=0, .reserved=0}};

uint64_t x;
spinlock lock;

void testfunc(uint64_t) {
  for(size_t i = 0; i < 10000000; ++i) {
    lock.lock();
    ++x;
    lock.unlock();
  }
  lock.lock();
  dbg::printf("{d}\n", x);
  lock.unlock();
  while(1);
}

extern "C"
[[noreturn]] void boot(uint64_t mbootheader) {
  pml4[0] = 0;
  mbootheader += VIRTUAL_OFFSET;
  dbg::printf("booting...\n");
  dbg::printf("clearing bss...\n");
  memset(bss_start, 0, bss_end-bss_start);

  loadTSS();

  dbg::printf("multiboot2 header at {}\n", (uint64_t *)mbootheader);
  
  dbg::printf("parsing multiboot...\n");
  mboot::parse(mbootheader);
  vmm::AddressSpace::kernel_page_table_ = ((uint64_t)pml4)-VIRTUAL_OFFSET;
  pmm::initPageManager();
  
  dbg::printf("boot stack: {}-{}\n",boot_stack, boot_stack+sizeof(boot_stack));
  dbg::printf("bss end at {}\n", (uint64_t *)bss_end);

  irq::initIdt();
  irq::initAPIC();

  auto thread = (Thread *)kmm::kmalloc(sizeof(Thread));
  auto thread2 = (Thread *)kmm::kmalloc(sizeof(Thread));
  memset(thread, 0, sizeof(Thread));
  memset(thread2, 0, sizeof(Thread));

  sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 1));
  sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 2));
  sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 3));

  sched::launch();

  dbg::panic("end of boot funtion reached\n");
}
