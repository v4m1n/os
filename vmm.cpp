module;
#include <cstdint>
#include <cstddef>

module vmm;
import cpu;
import string;
import pmm;
import debug;

extern "C" {
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pml4[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pdpt[512];
[[gnu::section(".data"), gnu::aligned(PAGE_SIZE)]] uint64_t pd[512];
}

extern "C" size_t max_addr;
extern "C" size_t core_init_page;
extern "C" size_t LS_Virt;


namespace vmm {
uint64_t AddressSpace::kernel_page_table_;

AddressSpace::AddressSpace() {
  pml4_ = pmm::allocZeroPFN()*PAGE_SIZE;
  memcpy(identAddress<void *>(pml4_), pml4, PAGE_SIZE);
}
AddressSpace::~AddressSpace() {
  auto pml4 = identAddress<uint64_t *>(pml4_);
  for (size_t pml4i = 0; pml4i < 256; ++pml4i) {

    if ((pml4[pml4i] & PRESENT) == 0) continue;
    auto pdpt = pageAddress<uint64_t *>(getPFN(pml4[pml4i]));

    for (size_t pdpti = 0; pdpti < 512; ++pdpti) {

      if ((pdpt[pdpti] & PRESENT) == 0) continue;
      auto pd = pageAddress<uint64_t *>(getPFN(pdpt[pdpti]));

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        if ((pd[pdi] & PRESENT) == 0) continue;
        auto pt = pageAddress<uint64_t *>(getPFN(pdpt[pdi]));

        for (size_t pti = 0; pti < 512; ++pti) {
          if ((pt[pti] & PRESENT) == 0) continue;
          pmm::freePFN(getPFN(pt[pti]));
        }
        pmm::freePFN(getPFN(pd[pdi]));
      }
      pmm::freePFN(getPFN(pdpt[pdpti]));
    }
    pmm::freePFN(getPFN(pml4[pml4i]));
  }
  pmm::freePFN(pml4_/PAGE_SIZE);
}

bool AddressSpace::mapPFN(const uint64_t vpn, const uint64_t pfn, const uint64_t writeable, const uint64_t nx) {
  const Offsets off{vpn};
  size_t *pdpt;
  size_t *pd;
  size_t *pt;
  auto pml4 = identAddress<uint64_t *>(pml4_);

  if ((pml4[off.pml4i] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pml4[off.pml4i] = setPFN(PRESENT|WRITEABLE|USER_ACCESS, pfn);
  }
  pdpt = pageAddress<uint64_t *>(getPFN(pml4[off.pml4i]));

  if ((pdpt[off.pdpti] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pdpt[off.pdpti] = setPFN(PRESENT|WRITEABLE|USER_ACCESS, pfn);
  }
  pd = pageAddress<uint64_t *>(getPFN(pdpt[off.pdpti]));

  if ((pd[off.pdi] & PRESENT) == 0) {
    size_t pfn = pmm::allocZeroPFN();
    pd[off.pdi] = setPFN(PRESENT|WRITEABLE|USER_ACCESS, pfn);
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

  if ((pml4[off.pml4i] & PRESENT) == 0) return -1;
  pdpt = pageAddress<uint64_t *>(getPFN(pml4[off.pml4i]));

  if ((pdpt[off.pdpti] & PRESENT) == 0) return -1;
  pd = pageAddress<uint64_t *>(getPFN(pdpt[off.pdpti]));

  if ((pd[off.pdi] & PRESENT) == 0) return -1;
  pt = pageAddress<uint64_t *>(getPFN(pd[off.pdi]));

  if (!(pt[off.pti] & PRESENT)) return -1;
  auto pfn = getPFN(pt[off.pti]); 

  pt[off.pti] = 0;

  if (!memzero(pt, PAGE_SIZE)) return pfn;
  auto pt_pfn = getPFN(pd[off.pdi]);
  pd[off.pdi] = 0;
  pmm::freePFN(pt_pfn);

  if (!memzero(pd, PAGE_SIZE)) return pfn;
  auto pd_pfn = getPFN(pdpt[off.pdpti]);
  pdpt[off.pdpti] = 0;
  pmm::freePFN(pd_pfn);
  
  if (!memzero(pdpt, PAGE_SIZE)) return pfn;
  auto pdpt_pfn = getPFN(pml4[off.pml4i]);
  pml4[off.pml4i] = 0;
  pmm::freePFN(pdpt_pfn);

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
  dbg::printf("initializing identity mapping...\n");
  constinit static bool called = false;
  dbg::panic_assert(!called, "initIdentityMapping has already been called\n");
  called = true;

  core_init_page = pmm::allocPFN();
  dbg::panic_assert(core_init_page*PAGE_SIZE < 64*1024ULL, "core init page above 1MB\n");

  dbg::printf("initializing identity mapping...\n");

  kernel_page_table_ = (reinterpret_cast<uint64_t>(pml4)-(size_t)&LS_Virt);

  const size_t max = (max_addr + (PAGE_SIZE-1))/PAGE_SIZE;
  constexpr size_t start = IDENTITY_MAPPING/PAGE_SIZE;
  constexpr Offsets start_off{start};
  Offsets end_off{max+start};
  dbg::panic_assert(max_addr <= 1024ULL*1024ULL*1024ULL*1024ULL, "physical address space is over 1TB\n");
  static_assert(!start_off.pti && !start_off.pdi && !start_off.pdpti);
  size_t pfn = 0;



  size_t cr3;
  asm ("mov %0, %%cr3":"=r"(cr3));
  dbg::printf("{} {}\n", pml4, cr3);

  for (size_t pml4i = start_off.pml4i; pml4i < end_off.pml4i; ++pml4i) {
    size_t pdpt_pfn = pmm::allocPFN();
    uint64_t *pdpt = (uint64_t *)(pdpt_pfn*PAGE_SIZE + (size_t)&LS_Virt);
    memset(pdpt, 0, PAGE_SIZE);
    dbg::panic_assert(pml4[pml4i] == 0, "pml4[pml4i] == 0\n");
    pml4[pml4i] = setPFN(PRESENT|WRITEABLE, pdpt_pfn);

    for (size_t pdpti = 0; pdpti < 512; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = (uint64_t *)(pd_pfn*PAGE_SIZE + (size_t)&LS_Virt);
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
    uint64_t *pdpt = (uint64_t *)(pdpt_pfn*PAGE_SIZE + (size_t)&LS_Virt);
    memset(pdpt, 0, PAGE_SIZE);
    dbg::panic_assert(pml4[end_off.pml4i] == 0, "pml4[end_off.pml4i] == 0\n");
    pml4[end_off.pml4i] = setPFN(PRESENT|WRITEABLE, pdpt_pfn);

    for (size_t pdpti = 0; pdpti <= end_off.pdpti; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = (uint64_t *)(pd_pfn*PAGE_SIZE + (size_t)&LS_Virt);
      memset(pd, 0, PAGE_SIZE);
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE, pd_pfn);

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|HUGE_PAGE, pfn);
        pfn += 512;
      }
    }
  }
}
void *AddressSpace::ioremap(const uint64_t pfn, const bool huge_page) {
  constexpr size_t start = IO_MAPPING/PAGE_SIZE;
  constexpr Offsets start_off{start};
  static_assert(!start_off.pti && !start_off.pdi && !start_off.pdpti);

  dbg::panic_assert(!huge_page || !(pfn&0x1ffULL), "pfn not correctly alligned\n");


  uint64_t *pdpt = pageAddress<size_t *>(getPFN(pml4[start_off.pml4i]));

  for (size_t pdpti = 0; pdpti < 512; ++pdpti) {
    if ((pdpt[pdpti] & PRESENT) == 0) {
      size_t pd_pfn = pmm::allocZeroPFN();
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pd_pfn);
    }
    uint64_t *pd = pageAddress<size_t *>(getPFN(pdpt[pdpti]));

    for (size_t pdi = 0; pdi < 512; ++pdi) {
      if ((pd[pdi] & PRESENT) && ((pd[pdi] & HUGE_PAGE) || huge_page)) continue;

      if (huge_page && !(pd[pdi] & PRESENT)) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED|HUGE_PAGE, pfn);
        return (void *)(IO_MAPPING | (pdpti << (12+9+9)) | pdi << (12+9));
      }
      if ((pd[pdi] & PRESENT) == 0) {
        size_t pt_pfn = pmm::allocZeroPFN();
        pd[pdi] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pt_pfn);
      }
      uint64_t *pt = pageAddress<size_t *>(getPFN(pd[pdi]));
      
      for (size_t pti = 0; pti < 512; ++pti) {
        if ((pt[pti] & PRESENT) == 0) {
          pt[pti] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pfn);
          return (void *)(IO_MAPPING | (pdpti << (12+9+9)) | pdi << (12+9) | (pti << 12));
        }
      }
    }
  }

  dbg::panic("io space full\n");
  return 0;
}
void AddressSpace::initIORemap() {
  constinit static bool called = false;
  dbg::panic_assert(!called, "initIORemap has already been called\n");

  dbg::printf("initializing IO mapping...\n");

  constexpr size_t start = IO_MAPPING/PAGE_SIZE;
  constexpr Offsets start_off{start};
  static_assert(!start_off.pti && !start_off.pdi && !start_off.pdpti);

  size_t pdpt_pfn = pmm::allocPFN();
  uint64_t *pdpt = pageAddress<uint64_t *>(pdpt_pfn);
  memset(pdpt, 0, PAGE_SIZE);
  dbg::panic_assert(pml4[start_off.pml4i] == 0, "pml4[pml4i] == 0\n");
  pml4[start_off.pml4i] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pdpt_pfn);
}

void AddressSpace::initUCIdentityMapping() {
  constinit static bool called = false;
  dbg::panic_assert(!called, "initIdentityMapping has already been called\n");
  called = true;

  dbg::printf("initializing uncached identity mapping...\n");

  const size_t max = (max_addr + (PAGE_SIZE-1))/PAGE_SIZE;
  constexpr size_t start = UC_IDENTITY_MAPPING/PAGE_SIZE;
  constexpr Offsets start_off{start};
  Offsets end_off{max+start};
  dbg::panic_assert(max_addr <= 1024ULL*1024ULL*1024ULL*1024ULL, "physical address space is over 1TB\n");
  static_assert(!start_off.pti && !start_off.pdi && !start_off.pdpti);
  size_t pfn = 0;


  for (size_t pml4i = start_off.pml4i; pml4i < end_off.pml4i; ++pml4i) {
    size_t pdpt_pfn = pmm::allocPFN();
    uint64_t *pdpt = pageAddress<uint64_t *>(pdpt_pfn);
    memset(pdpt, 0, PAGE_SIZE);
    dbg::panic_assert(pml4[pml4i] == 0, "pml4[pml4i] == 0\n");
    pml4[pml4i] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pdpt_pfn);

    for (size_t pdpti = 0; pdpti < 512; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = pageAddress<uint64_t *>(pd_pfn);
      memset(pd, 0, PAGE_SIZE);
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pd_pfn);

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED|HUGE_PAGE, pfn);
        pfn += PAGE_SIZE*512;
      }
    }
  }

  {
    size_t pdpt_pfn = pmm::allocPFN();
    uint64_t *pdpt = pageAddress<uint64_t *>(pdpt_pfn);
    memset(pdpt, 0, PAGE_SIZE);
    dbg::panic_assert(pml4[end_off.pml4i] == 0, "pml4[end_off.pml4i] == 0\n");
    pml4[end_off.pml4i] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pdpt_pfn);

    for (size_t pdpti = 0; pdpti <= end_off.pdpti; ++pdpti) {
      size_t pd_pfn = pmm::allocPFN();
      uint64_t *pd = pageAddress<uint64_t *>(pd_pfn);
      memset(pd, 0, PAGE_SIZE);
      pdpt[pdpti] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED, pd_pfn);

      for (size_t pdi = 0; pdi < 512; ++pdi) {
        pd[pdi] = setPFN(PRESENT|WRITEABLE|WRITE_THROUGH|CACHE_DISABLED|HUGE_PAGE, pfn);
        pfn += 512;
      }
    }
  }
}
};

