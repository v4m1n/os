#include "stdint.h"

constexpr size_t IDENT_OFFSET = 0xffff800000000000ULL;

namespace vmm {
void *pfnAddress(size_t pfn) {
  return reinterpret_cast<void *>(IDENT_OFFSET | pfn);
}


class AddressSpace {
  public:
    AddressSpace(AddressSpace &) = delete;
    AddressSpace(AddressSpace &&) = delete;
    AddressSpace &operator=(AddressSpace &) = delete;
    AddressSpace &operator=(AddressSpace &&) = delete;
  private:
};

};
