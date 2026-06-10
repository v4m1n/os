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
#include "pci.h"
#include "asm.h"
#include "driver/block.h"
#include "vfs.h"
#include "driver/fat32.h"


#include <stdint.h>

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

  {
    struct PartitionTableEntry {
      uint8_t attributes_;
      uint8_t partition_start_chs_[3];
      uint8_t partition_type_;
      uint8_t last_sector_chs_[3];
      uint32_t partition_start_lba_;
      uint32_t number_of_sections_;
    } __attribute__((packed));

    struct MBR {
      uint8_t bootstrap_[440];
      uint32_t disk_id_;
      uint16_t reserved_;
      PartitionTableEntry partition_table_[4];
      uint16_t signature_;
    } __attribute__((packed));

    BlockDevice *dev = block::getDevice(0);
    if (dev) {
      MBR mbr;
      if (dev->readBlocks(0, 1, &mbr) == 0 && mbr.signature_ == 0xAA55) {
        for (int i = 0; i < 4; ++i) {
          auto &entry = mbr.partition_table_[i];
          if (entry.partition_type_ != 0) {
            dbg::printf("Found partition {}: type={}, start_lba={}\n",
                        (uint64_t)i, (uint64_t)entry.partition_type_, (uint64_t)entry.partition_start_lba_);
            auto *fs = new fat32::FAT32FileSystem(dev, entry.partition_start_lba_);
            if (fs->init()) {
              vfs::mount("/", fs);
              dbg::printf("Mounted FAT32 filesystem from partition {} to /\n", (uint64_t)i);
              
              vfs::VfsNode *node = vfs::open("/boot/grub/grub.cfg");
              if (node) {
                dbg::printf("Opened /boot/grub/grub.cfg successfully! Size: {} bytes\n", (uint64_t)node->getSize());
                char *buf = reinterpret_cast<char *>(kmm::kmalloc(node->getSize() + 1));
                if (buf) {
                  int read_bytes = node->read(0, node->getSize(), buf);
                  if (read_bytes > 0) {
                    buf[read_bytes] = '\0';
                    dbg::printf("--- grub.cfg Content ---\n{}\n-----------------------\n", buf);
                  }
                  kmm::kfree(buf);
                }

                // Test write support
                const char *test_str = "\n# Hello from the FAT32 Write Driver!\n";
                uint32_t write_len = strlen(test_str);
                uint64_t original_size = node->getSize();
                int written = node->write(original_size, write_len, test_str);
                if (written == (int)write_len) {
                  dbg::printf("Successfully wrote to /boot/grub/grub.cfg! New size: {} bytes\n", (uint64_t)node->getSize());
                  
                  // Read back to verify
                  char *new_buf = reinterpret_cast<char *>(kmm::kmalloc(node->getSize() + 1));
                  if (new_buf) {
                    int read_back = node->read(0, node->getSize(), new_buf);
                    if (read_back > 0) {
                      new_buf[read_back] = '\0';
                      dbg::printf("--- Verification of New Content ---\n{}\n-----------------------\n", new_buf);
                    }
                    kmm::kfree(new_buf);
                  }
                } else {
                  dbg::printf("Failed to write to file: returned {}\n", (int64_t)written);
                }

                delete node;
              } else {
                dbg::printf("Failed to open /boot/grub/grub.cfg\n");
              }
              break;
            } else {
              delete fs;
            }
          }
        }
      } else {
        dbg::printf("MBR signature invalid or read failed\n");
      }
    } else {
      dbg::printf("No block devices registered\n");
    }
  }
  irq::launchCores();
  pml4[0] = 0;
  pdpt[0] = 0;

  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 1));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 2));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 3));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 4));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 5));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 6));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 7));
  //sched::addThread(sched::createKernelThread(reinterpret_cast<size_t>(testfunc), 8));
  sched::addThread(sched::createUserThread(0, 8));
  sched::addThread(sched::createUserThread(1, 0xdead));

  sched::launch();

  dbg::panic("end of boot funtion reached\n");
}
