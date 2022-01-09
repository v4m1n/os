#include "pmm.h"
#include "stdint.h"
#include "debug.h"
#include "string.h"
#include "vmm.h"

extern size_t KERNEL_START;
extern size_t KERNEL_END;
extern size_t VIRTUAL_OFFSET;
extern size_t max_addr;

namespace pmm {
MemoryRegion mem_regions[16];
static uint8_t *bitmap_;
static size_t bitmap_size_;
static size_t lowest_free_;
static size_t num_pages_;

void initPageManager() { 
  static size_t call = 0;
  dbg::panic_assert(call++ == 0, "trying to construct a second PageManager\n");
  num_pages_ = (max_addr+PAGE_SIZE-1)/PAGE_SIZE;
  //bitmap_ = reinterpret_cast<uint8_t *>(KERNEL_END);
  //dbg::panic_assert(bitmap+num_pages_/8 <= VIRTUAL_OFFSET)
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


 
  
  const size_t start = (KERNEL_START-VIRTUAL_OFFSET)/PAGE_SIZE;
  const size_t end = ((KERNEL_END-VIRTUAL_OFFSET)+PAGE_SIZE-1)/PAGE_SIZE;
 
  for (size_t j = start; j < end; ++j) {
    if (j >= 8ULL*sizeof(init_bitmap)) {
      break;
    }
    init_bitmap[j/8] |= (1U<<(j%8));
  }

  dbg::panic_assert((init_bitmap[0] & 1) == 0, "core init page not free\n");

  vmm::AddressSpace::initIdentityMapping();

  size_t pm_start = end;
  for (;init_bitmap[pm_start/8]&(1U<<(pm_start%8)) && pm_start < bitmap_size_; ++pm_start);
  size_t pm_end = pm_start;
  for (size_t cnt = 0; !(init_bitmap[pm_end/8]&(1U<<(pm_end%8))) && cnt < num_pages_ && pm_end < bitmap_size_; ++pm_end, cnt += PAGE_SIZE*8) 
    init_bitmap[pm_end/8] |= (1U<<(pm_end%8));

  dbg::panic_assert(pm_start != pm_end, "no space for pm\n");

  bitmap_ = vmm::pageAddress<uint8_t *>(pm_start);
  dbg::printf("bitmap {x}: {x}-{x}\n", bitmap_, pm_start, pm_end);
  bitmap_size_ = (pm_end-pm_start) * PAGE_SIZE * 8;
  memset(bitmap_, 0xff, bitmap_size_/8);

  dbg::printf("bitmap_size {}\n", bitmap_size_);

  for (size_t i = 0; i < 16; ++i) {
    const auto &m = mem_regions[i];
    const size_t start = (m.addr+PAGE_SIZE-1)/PAGE_SIZE;
    const size_t end = (m.addr+m.len)/PAGE_SIZE;
    for (size_t j = start; j < end; ++j) {
      bitmap_[j/8] &= ~(1U<<(j%8));
    }
  }
  memcpy(bitmap_, init_bitmap, PAGE_SIZE);

  vmm::AddressSpace::initUCIdentityMapping();
}

size_t allocZeroPFN(size_t size) {
  size_t pfn = allocPFN(size);
  memset(vmm::pageAddress<void *>(pfn), 0, PAGE_SIZE);
  return pfn;
}

size_t allocPFN(size_t size) {
  dbg::panic_assert(size && !(size&(PAGE_SIZE-1)), "size must be multiple of page size\n");
  const size_t pages = size/PAGE_SIZE;
  size_t cnt = 0;
  for (size_t i = lowest_free_; i < bitmap_size_-pages+1; ++i) {
    const size_t idx = i/8;
    const size_t offset = i%8;
    if (!offset && bitmap_[idx] == 0xffU) {
      i += 8;
      cnt = 0;
      continue;
    }
    if (bitmap_[idx] & (1U<<offset)) {
      cnt = 0;
      continue;
    }
    if (++cnt != pages)
      continue;
    size_t pfn = i-cnt+1;
    for (size_t j = pfn; j <= i; ++j) {
      bitmap_[j/8] |= (1U<<(j%8));
    }
    if (pfn == lowest_free_)
      lowest_free_ = i+1;
    return pfn;
  }
  dbg::panic("out of pages\n");
}
void freePFN(size_t pfn, size_t size) {
  while(size--) {
    const size_t idx = pfn/8;
    const size_t offset = pfn%8;
    dbg::panic_assert(pfn < bitmap_size_, "free pfn, pfn too large {}\n", pfn);
    dbg::panic_assert(bitmap_[idx] & (1U<<offset), "invalid free pfn\n");
    bitmap_[idx] &= ~(1U<<offset);
    ++pfn;
  }
  if (pfn < lowest_free_)
    lowest_free_ = pfn;
}

}

