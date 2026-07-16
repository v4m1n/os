module;
#include <cstdint>
#include <cstddef>

export module vmm;
import pmm;
import vfs;
import sync;

extern "C" {
export extern uint64_t pml4[512];
export extern uint64_t pdpt[512];
export extern uint64_t pd[512];
}

export namespace vmm {

class AddressSpace {
  public:

    constexpr static size_t PRESENT = (1ULL<<0);
    constexpr static size_t WRITEABLE = (1ULL<<1);
    constexpr static size_t USER_ACCESS = (1ULL<<2);
    constexpr static size_t WRITE_THROUGH = (1ULL<<3);
    constexpr static size_t CACHE_DISABLED = (1ULL<<4);
    constexpr static size_t ACCESSED = (1ULL<<5);
    constexpr static size_t DIRTY = (1ULL<<6);
    constexpr static size_t HUGE_PAGE = (1ULL<<7);
    constexpr static size_t GLOBAL = (1ULL<<8);
    constexpr static size_t EXECUTION_DISABLED = (1ULL<<63);
    constexpr static uint64_t IDENTITY_MAPPING = 0xffff'8000'0000'0000ULL;
    constexpr static uint64_t UC_IDENTITY_MAPPING = 0xffff'a000'0000'0000ULL;
    constexpr static uint64_t IO_MAPPING = 0xffff'c000'0000'0000ULL;
    constexpr static uint64_t USER_END = 0x0000'8000'0000'0000ULL;

    static void initIdentityMapping();
    static void initUCIdentityMapping();
    static void initIORemap();

    static bool mapKernelPFN(const uint64_t vpn, const uint64_t pfn, const uint64_t writeable=1, const uint64_t nx=0);

    static void setKernelCaching(const size_t vpn, const uint64_t cache);

    static void *ioremap(const uint64_t pfn, const bool huge_page=false);

    static inline size_t setPFN(const uint64_t entry, const size_t pfn) {
      return (entry&~0xffffffffff000ULL)|(pfn<<12);
    }
    static inline size_t getPFN(const uint64_t entry) {
      return (entry&0xffffffffff000ULL)>>12;
    }

    static uint64_t kernel_page_table_;
    static mutex kernel_address_space_lock_;

    mutex address_space_lock_;

    AddressSpace();
    ~AddressSpace();

    AddressSpace(AddressSpace &) = delete;
    AddressSpace(AddressSpace &&) = delete;
    AddressSpace &operator=(AddressSpace &) = delete;
    AddressSpace &operator=(AddressSpace &&) = delete;

    bool mapPFN(const uint64_t vpn, const uint64_t pfn, const uint64_t writeable, const uint64_t nx);
    uint64_t unmapPFN(const uint64_t vpn);

    uint64_t pml4_;

  private:
    struct Offsets {
      const size_t pti;
      const size_t pdi;
      const size_t pdpti;
      const size_t pml4i;

      Offsets() = delete;
      constexpr Offsets(size_t vpn) : pti(vpn & 0x1ff), 
                                      pdi((vpn >> 9) & 0x1ff), 
                                      pdpti((vpn >> 18) & 0x1ff), 
                                      pml4i((vpn >> 27) & 0x1ff) {}
    };
};

template<typename T>
inline T pageAddress(size_t pfn) {
  return reinterpret_cast<T>(AddressSpace::IDENTITY_MAPPING | pfn*PAGE_SIZE);
}
template<typename T>
inline T identAddress(size_t addr) {
  return reinterpret_cast<T>(AddressSpace::IDENTITY_MAPPING | addr);
}
template<typename T>
inline T pageUCAddress(size_t pfn) {
  return reinterpret_cast<T>(AddressSpace::UC_IDENTITY_MAPPING | pfn*PAGE_SIZE);
}
template<typename T>
inline T identUCAddress(size_t addr) {
  return reinterpret_cast<T>(AddressSpace::UC_IDENTITY_MAPPING | addr);
}

}
