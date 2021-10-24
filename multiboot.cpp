#include "multiboot2.h"
#include "debug.h"
#include "string.h"
#include "stdint.h"

namespace mboot {

size_t memory_info_size;
multiboot_mmap_entry memory_info[16];



void parse(uint64_t header) {
  uint32_t size = *(uint32_t*)header;
  dbg::printf("multiboot header size {}\n", size);
  for (size_t i = header+8; i < header+size;) {
    uint32_t type = *(uint32_t*)i;
    uint64_t tag_size = *(uint32_t*)(i+4);
    switch(type) {
      case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
        {
          auto m = (multiboot_tag_basic_meminfo *)i;
          dbg::printf("basic memory info: 0 - {}, {} - {}\n", m->mem_lower*1024, 1024U*1024U, m->mem_upper*1024);
        }
        break;
      case MULTIBOOT_TAG_TYPE_BOOTDEV: //get boot device
        {
          auto dev = (multiboot_tag_bootdev *)i;
          dbg::printf("boot device: biosdev: {}, partition:  {}, sub_part: {}\n", dev->biosdev, dev->slice, dev->part);
        }
        break;
      case MULTIBOOT_TAG_TYPE_CMDLINE:
        {
          auto s = (multiboot_tag_string *)i;
          dbg::printf("cmd line: {}\n", s->string);
        }
        break;
      case MULTIBOOT_TAG_TYPE_MMAP:
        {
          auto m = (multiboot_tag_mmap *)i;
          const size_t num = (m->size-16)/m->entry_size;
          dbg::printf("memory map:\n");
          dbg::panic_assert(num <= 16, "too many memory regions\n");
          memory_info_size = num;
          memcpy(memory_info, m->entries, sizeof(multiboot_mmap_entry)*num);
          for (size_t j = 0; j < num; ++j) {
            auto &e = m->entries[j];
            dbg::printf("  memory type {}: {} - {}\n", e.type, e.addr, e.addr+e.len);
          }
        }
        break;
      case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
        {
          auto s = (multiboot_tag_string *)i;
          dbg::printf("bootloader: {}\n", s->string);
        }
        break;
      case MULTIBOOT_TAG_TYPE_VBE:
        {
          auto vbe = (multiboot_tag_vbe *)i;
          dbg::printf("vbe mode: {}\n", vbe->vbe_mode);
        }
        break;
      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
        {
          auto fb = (multiboot_tag_framebuffer_common *)i;
          dbg::printf("framebuffer:\n");
          dbg::printf("  addr: {}\n", fb->framebuffer_addr);
          dbg::printf("  pitch: {}, bpp: {}, type: {}\n", fb->framebuffer_pitch, fb->framebuffer_bpp, fb->framebuffer_type);
          dbg::printf("  res: {d}x{d}\n", fb->framebuffer_width, fb->framebuffer_height);
        }
        break;
      case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
        {
          auto load_addr = (multiboot_tag_load_base_addr *)i;
          dbg::printf("load address: {}\n", load_addr->load_base_addr);
          dbg::panic_assert(load_addr->load_base_addr == 0x100000U, "kernel not loaded to the correct address\n");
        }
        break;
      default:
        dbg::printf("ignoring tag type {d}\n", type);
        break;
    }
    i += tag_size;
    i = (i + 7) & ~7ULL;
  }

}

}
