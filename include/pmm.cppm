module;
#include <cstdint>
#include <cstddef>

export module pmm;

export constexpr size_t PAGE_SIZE = 4096ULL;

export namespace pmm {
struct MemoryRegion {
  size_t addr;
  size_t len;
};
struct Page {
};

void initPageManager();
[[nodiscard]] size_t allocPFN(size_t size=PAGE_SIZE);
[[nodiscard]] size_t allocZeroPFN(size_t size=PAGE_SIZE);
void freePFN(size_t pfn, size_t size=PAGE_SIZE);

extern MemoryRegion mem_regions[64];

}
