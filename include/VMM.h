#include "stdint.h"

constexpr size_t IDENT_OFFSET = 0xffff800000000000ULL;

extern uint64_t pml4[512];
extern uint64_t pdpt[512];
extern uint64_t pd[512];

namespace vmm {
inline void *pfnAddress(size_t pfn) {
  return reinterpret_cast<void *>(IDENT_OFFSET | pfn);
}


class AddressSpace {
  public:
    constexpr static size_t PRESENT = (1ULL<<0);
    constexpr static size_t WRITEABLE = (1ULL<<1);
    constexpr static size_t USER_ACCESS = (1ULL<<2);
    constexpr static size_t ACCESSED = (1ULL<<5);
    constexpr static size_t DIRTY = (1ULL<<6);
    constexpr static size_t HUGE_PAGE = (1ULL<<7);
    constexpr static size_t GLOBAL = (1ULL<<8);
    constexpr static size_t EXECUTION_DISABLED = (1ULL<<63);
    static inline size_t setPFN(const uint64_t entry, const size_t pfn) {
      return (entry&~0xffffffffff000ULL)|pfn;
    }
    static inline size_t getPFN(const uint64_t entry) {
      return (entry&0xffffffffff000ULL)>>12;
    }

    static uint64_t kernel_page_table_;

    AddressSpace(AddressSpace &) = delete;
    AddressSpace(AddressSpace &&) = delete;
    AddressSpace &operator=(AddressSpace &) = delete;
    AddressSpace &operator=(AddressSpace &&) = delete;
  private:
};

};
