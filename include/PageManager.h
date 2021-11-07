#pragma once
#include "stdint.h"

namespace pm {
struct MemoryRegion {
  size_t addr;
  size_t len;
};


class PageManager {
  public:
    PageManager();

    PageManager(PageManager &) = delete;
    PageManager(PageManager &&) = delete;
    PageManager &operator=(PageManager &) = delete;
    PageManager &operator=(PageManager &&) = delete;

    [[nodiscard]] size_t allocPFN();
    void freePFN(size_t pfn);

  private:

    uint8_t *bitmap_;
    size_t bitmap_size_;
    size_t lowest_free_;
};

extern PageManager instance;
extern MemoryRegion mem_regions[16];

}
