#include <cstddef>
#include <cstdint>
#include <ranges>


import knew;
import cpu;
import scheduler;
import block;
import vfs;
import fat32;
import string;
import gdt;
import kmm;
import sync;
import pmm;
import debug;
import vmm;
import registers;
import interrupts;
import pci;
import multiboot;
import loader;
import utility;

[[maybe_unused]] const volatile struct __attribute__((packed))
{
    unsigned int magic = 0xe85250d6;
    unsigned int flags = 0;
    unsigned int size = 4*9;
    unsigned int checksum = -(magic+size);
    struct __attribute__((packed)) {
        unsigned short type = 5;
        unsigned short flags = 0;
        unsigned int size = 20;
        unsigned int widht = 0;
        unsigned int height = 0;
        unsigned int depth = 0;
    } framebuffer;
    unsigned int end_tag[3] = {0,0,8};
} multiboot  __attribute__ ((section (".multiboot")));


extern "C" {
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]]
uint8_t boot_stack[PAGE_SIZE*2];

extern size_t LS_Virt;
extern uint8_t *bss_end[];
extern uint8_t *bss_start[];
extern size_t kernel_start;
extern size_t kernel_end;

GDTE gdt[7] = {{},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=0,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=1, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=1, .size=0, .granularity=1, .base3=0},
               {.limit1=0xffffU, .base1=0, .base2=0, .accessed=0, .rw=1, .direction=0, .executable=0, .descriptor=1, .priv=3,
                .present=1, .limit2=0xfU, .zero=0, .lmode=0, .size=1, .granularity=1, .base3=0},
               {.limit1=sizeof(TSS)-1, .base1=0, .base2=0, .accessed=1, .rw=0, .direction=0, .executable=1, .descriptor=0, .priv=0,
                .present=1, .limit2=0, .zero=0, .lmode=0, .size=1, .granularity=0, .base3=0}};
}

uint64_t x;
spinlock lock;

void testfunc(uint64_t j) {
  for(size_t i = 0; i < 10000; ++i) {
    dbg::putchar('A'+j);
    
    lock.lock();
    ++x;
    lock.unlock();
  }
  lock.lock();
  dbg::printf("{d}\n", x);
  lock.unlock();
  while(1) hlt();
}

extern "C"
[[noreturn]] void boot(uint64_t mbootheader) {
  mbootheader += (size_t)&LS_Virt;
  dbg::printf("booting...\n");
  dbg::printf("clearing bss...\n");


  memset(bss_start, 0, bss_end-bss_start);

  dbg::printf("multiboot2 header at {}\n", (uint64_t *)mbootheader);
  
  dbg::printf("parsing multiboot...\n");
  mboot::parse(mbootheader);





  //dbg::printf("parsing MPT...\n");
  //irq::parseMPT();
  dbg::printf("init pagemanager...\n");
  vmm::AddressSpace::kernel_page_table_ = ((uint64_t)pml4)-(size_t)&LS_Virt;
  pmm::initPageManager();
  kmm::kmalloc_init();
  


  dbg::printf("boot stack: {}-{}\n",boot_stack, boot_stack+sizeof(boot_stack));
  dbg::printf("bss end at {}\n", (uint64_t *)bss_end);

  irq::initIdt();
  irq::remapDisablePIC();
  irq::initAPIC();
  irq::parseRSDT();
  pci::deviceDetection();
  irq::launchCores();

  pml4[0] = 0;
  pdpt[0] = 0;

  for (uint32_t i = 0; i < block::getDeviceCount(); ++i) {
    block::detectAndMountPartitions(block::getDevice(i));
  }

  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 1));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 2));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 3));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 4));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 5));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 6));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 7));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 8));

  {
    util::shared_ptr loader(new Loader());
    dbg::panic_assert(loader->init("/bin/init"), "init setup failed");
    sched::addThread(sched::createUserThread(loader->entry_, 0, loader));

  }
  //sched::addThread(sched::createUserThread(1, 0xdead));
  dbg::printf("starting scheduler...\n");

  sched::launch();

  dbg::panic("end of boot funtion reached\n");
}
