#include "PageManager.h"
#include "stdint.h"
#include "debug.h"
#include "string.h"

extern size_t KERNEL_START;
extern size_t KERNEL_END;
extern size_t VIRTUAL_OFFSET;

namespace pm {
PageManager instance;
MemoryRegion mem_regions[16];

PageManager::PageManager() {
  static size_t call = 0;
  dbg::panic_assert(call++ == 0, "trying to construct a second PageManager\n");
  uint8_t init_bitmap[PAGE_SIZE] = {0};
  memset(init_bitmap, 0xffU, sizeof(init_bitmap));
  bitmap_ = init_bitmap;
  bitmap_size_ = PAGE_SIZE*8;
  lowest_free_ = 0;

  for (size_t i = 0; i < 16; ++i) {
    const auto &m = mem_regions[i];
    const size_t start = (m.addr+PAGE_SIZE-1)/PAGE_SIZE;
    const size_t end = (m.addr+m.len)/PAGE_SIZE;
    if (start >= 8ULL*sizeof(init_bitmap)) {
      continue;
    }
    for (size_t j = start; j < end; ++j) {
      if (j >= 8ULL*sizeof(init_bitmap)) {
        break;
      }
      init_bitmap[j/8] &= ~(1U<<(j%8));
    }
  }
  {
    const size_t start = (KERNEL_START-VIRTUAL_OFFSET)/PAGE_SIZE;
    const size_t end = ((KERNEL_END-VIRTUAL_OFFSET)+PAGE_SIZE-1)/PAGE_SIZE;
    for (size_t j = start; j < end; ++j) {
      if (j >= 8ULL*sizeof(init_bitmap)) {
        break;
      }
      init_bitmap[j/8] |= (1U<<(j%8));
    }
  }
  /*for (size_t i = 0; i < sizeof(init_bitmap); i+=16) {
    for (size_t j = 0; j < 16; ++j)
      dbg::printf("{x} ", init_bitmap[i+j]);
    dbg::printf("\n");
  }*/


}



size_t PageManager::allocPFN() {
  for (size_t i = lowest_free_; i < bitmap_size_/8; ++i) {
    if (bitmap_[i] == 0xffU)
      continue;
    for (size_t j = 0; j < 8; ++j) {
      if (bitmap_[i] & (1U<<j))
        continue;
      bitmap_[i] |= (1U<<j);
      if (bitmap_[i] == 0xffU) ++lowest_free_;
      return (i*8) + j;
    }
  }
  dbg::panic_assert(0, "out of pages\n");
  return -1ULL;
}
void PageManager::freePFN(size_t pfn) {
  dbg::panic_assert(bitmap_[pfn/8] & (1U<<(pfn%8)), "invalid free pfn\n");
  bitmap_[pfn/8] &= ~(1U<<(pfn%8));
  if (pfn/8 < lowest_free_)
    lowest_free_ = pfn/8;
}

}

