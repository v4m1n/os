#include "vmm.h"
#include "pmm.h"
#include "debug.h"
#include "string.h"
#include "asm.h"

[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pml4[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pdpt[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pd[512];

extern size_t max_addr;
extern size_t VIRTUAL_OFFSET;
extern size_t core_init_page;

namespace vmm {
uint64_t AddressSpace::kernel_page_table_;

AddressSpace::AddressSpace() {
  pml4_ = pmm::allocZeroPFN()*PAGE_SIZE;
  memcpy(identAddress<void *>(pml4_), pml4, PAGE_SIZE);
}

bool AddressSpace::mapPFN(const uint64_t vpn, const uint64_t pfn, const uint64_t writeable, const uint64_t nx) {
  const Offsets off{vpn};
  size_t *pdpt;
  size_t *pd;
  size_t *pt;
  auto pml4 = identAddress<uint64_t *>(pml4_);

  if ((pml4[off.pml4i] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pml4[off.pml4i] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pdpt = pageAddress<uint64_t *>(getPFN(pml4[off.pml4i]));

  if ((pdpt[off.pdpti] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pdpt[off.pdpti] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pd = pageAddress<uint64_t *>(getPFN(pdpt[off.pdpti]));

  if ((pd[off.pdi] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pd[off.pdi] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pt = pageAddress<uint64_t *>(getPFN(pd[off.pdi]));

  if (pt[off.pti] & PRESENT) {
    return false;
  }
  dbg::panic_assert(pt[off.pti] == 0, "mapping pfn to a non zero entry\n");
  pt[off.pti] = setPFN(0, pfn);
  pt[off.pti] |= writeable ? WRITEABLE : 0;
  pt[off.pti] |= nx ? EXECUTION_DISABLED : 0;
  pt[off.pti] |= USER_ACCESS;
  pt[off.pti] |= PRESENT;
  return true;
}

uint64_t AddressSpace::unmapPFN(const uint64_t vpn) {
  const Offsets off{vpn};
  size_t *pdpt;
  size_t *pd;
  size_t *pt;
  auto pml4 = identAddress<uint64_t *>(pml4_);

  if ((pml4[off.pml4i] & PRESENT) == 0) {
    return -1;
  }
  pdpt = pageAddress<uint64_t *>(getPFN(pml4[off.pml4i]));

  if ((pdpt[off.pdpti] & PRESENT) == 0) {
    return -1;
  }
  pd = pageAddress<uint64_t *>(getPFN(pdpt[off.pdpti]));

  if ((pd[off.pdi] & PRESENT) == 0) {
    return -1;
  }
  pt = pageAddress<uint64_t *>(getPFN(pd[off.pdi]));

  if (!(pt[off.pti] & PRESENT)) {
    return -1;
  }

  auto pfn = getPFN(pt[off.pti]); 
  pt[off.pti] = 0;
  return pfn;
}

bool AddressSpace::mapKernelPFN(const uint64_t vpn, const uint64_t pfn, const uint64_t writeable, const uint64_t nx) {
  const Offsets off{vpn};
  size_t *pdpt;
  size_t *pd;
  size_t *pt;

  if ((pml4[off.pml4i] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pml4[off.pml4i] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pdpt = pageAddress<size_t *>(getPFN(pml4[off.pml4i]));
  

  if ((pdpt[off.pdpti] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pdpt[off.pdpti] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pd = pageAddress<size_t *>(getPFN(pdpt[off.pdpti]));


  if ((pd[off.pdi] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pd[off.pdi] = setPFN(PRESENT|WRITEABLE, pfn);
  }
  pt = pageAddress<size_t *>(getPFN(pd[off.pdi]));

  if (pt[off.pti] & PRESENT) {
    return false;
  }
  pt[off.pti] = setPFN(0, pfn);
  pt[off.pti] |= writeable ? WRITEABLE : 0;
  pt[off.pti] |= nx ? EXECUTION_DISABLED : 0;
  pt[off.pti] |= PRESENT;
  return true;
}

void AddressSpace::setKernelCaching(const size_t vpn, const uint64_t cache) {
  const Offsets off{vpn};
  size_t *pdpt;
  size_t *pd;
  size_t *pt;

  dbg::panic_assert(pml4[off.pml4i] & PRESENT, "pdpt not present\n");
  pdpt = pageAddress<size_t *>(getPFN(pml4[off.pml4i]));
  
  dbg::panic_assert(pdpt[off.pdpti] & PRESENT, "pd not present\n");
  if (pdpt[off.pdpti] & HUGE_PAGE) {
    if (!cache)
      pdpt[off.pdpti] |= WRITE_THROUGH | CACHE_DISABLED;
    else 
      pdpt[off.pdpti] &= ~(WRITE_THROUGH | CACHE_DISABLED);
    invlpg(vpn*PAGE_SIZE);
    return;
  }
  pd = pageAddress<size_t *>(getPFN(pdpt[off.pdpti]));

  dbg::panic_assert(pd[off.pdi] & PRESENT, "pt not present\n");

  if (pd[off.pdi] & HUGE_PAGE) {
    if (!cache)
      pd[off.pdi] |= WRITE_THROUGH | CACHE_DISABLED;
    else 
      pd[off.pdi] &= ~(WRITE_THROUGH | CACHE_DISABLED);
    invlpg(vpn*PAGE_SIZE);
    return;
  }

  pt = pageAddress<size_t *>(getPFN(pdpt[off.pdpti]));

  dbg::panic_assert(pt[off.pti] & PRESENT, "page not present\n");

  if (!cache)
    pt[off.pti] |= WRITE_THROUGH | CACHE_DISABLED;
  else 
    pt[off.pti] &= ~(WRITE_THROUGH | CACHE_DISABLED);
  invlpg(vpn*PAGE_SIZE);
}

void AddressSpace::initIdentityMapping() {
  constinit static bool called = false;
  dbg::panic_assert(!called, "initIdentityMapping has already been called\n");
  called = true;

  core_init_page = pmm::allocPFN();
  dbg::panic_assert(core_init_page*PAGE_SIZE < 64*1024ULL, "core init page above 1MB\n");

  dbg::printf("initializing identity mapping...\n");

  kernel_page_table_ = (reinterpret_cast<uint64_t>(pml4)-VIRTUAL_OFFSET);

  const size_t max = (max_addr + (PAGE_SIZE-1))/PAGE_SIZE;
  constexpr size_t start = IDENTITY_MAPPING/PAGE_SIZE;
  constexpr Offsets start_off{start};
  Offsets end_off{max+start};
  dbg::panic_assert(max_addr <= 1024ULL*1024ULL*1024ULL*1024ULL, "physical address space is over 1TB\n");
  static_assert(!start_off.pti && !start_off.pdi && !start_off.pdpti);
  size_t pfn = 0;

  for (size_t pml4i = start_off.pml4i; pml4i < end_off.pml4i; ++pml4i) {
    size_t pdpt_pfn = pmm::allocPFN();
    uint64_t *pdpt = (uint64_t *)(pdpt_pfn*PAGE_SIZE + VIRTUAL_OFFSET);
    memset(pdpt, 0, PAGE_SIZE);
    pml4[pml4i] = setPFN(PRESENT|WRITEABLE, pdpt_pfn);

    for (size_t pdpti = 0; pdpti < 512; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = (uint64_t *)(pd_pfn*PAGE_SIZE + VIRTUAL_OFFSET);
      memset(pd, 0, PAGE_SIZE);
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE, pd_pfn);

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|HUGE_PAGE, pfn);
        pfn += PAGE_SIZE*512;
      }
    }
  }

  {
    size_t pdpt_pfn = pmm::allocPFN();
    uint64_t *pdpt = (uint64_t *)(pdpt_pfn*PAGE_SIZE + VIRTUAL_OFFSET);
    memset(pdpt, 0, PAGE_SIZE);
    pml4[end_off.pml4i] = setPFN(PRESENT|WRITEABLE, pdpt_pfn);

    for (size_t pdpti = 0; pdpti <= end_off.pdpti; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = (uint64_t *)(pd_pfn*PAGE_SIZE + VIRTUAL_OFFSET);
      memset(pd, 0, PAGE_SIZE);
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE, pd_pfn);

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|HUGE_PAGE, pfn);
        pfn += 512;
      }
    }
  }
}

};

