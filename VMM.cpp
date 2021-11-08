#include "VMM.h"
#include "PageManager.h"

[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pml4[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pdpt[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pd[512];

namespace vmm {
uint64_t AddressSpace::kernel_page_table_;

};

