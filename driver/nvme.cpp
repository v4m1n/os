#include "pci.h"
#include "nvme.h"
#include "debug.h"
#include "vmm.h"

struct NVMeBar0 {
  uint64_t capabilities_;
  uint32_t version_;
  uint32_t int_mask_set_;
  uint32_t int_mask_clear_;
  uint32_t controller_conf_;
  uint32_t unused_;
  uint32_t controller_status_;
  uint32_t reset_;
  uint32_t admin_queue_attr_;
  uint64_t admin_sub_queue_;
  uint64_t admin_comp_queue_;
};

static_assert(sizeof(NVMeBar0) == 0x38, "NVMeBar0 is not the correct size");

NVMe::NVMe(uint8_t bus, uint8_t dev) : bus_(bus), dev_(dev) {
  uint64_t bar1 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x10);
  uint64_t bar2 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x18);
  dbg::panic_assert(bar1 != -1U && bar2 != -1U, "nvme device does not exist ({}, {})\n", bus, dev);
  bar_phys_ = (bar1 & ~0xfU) | bar2<<32;
  dbg::panic_assert(bar_phys_&~(PAGE_SIZE-1), "nvme device mmio not page aligned {}\n", bar_phys_);
  dbg::printf("nvme mmio {}\n", bar_phys_);
  vmm::AddressSpace::setKernelCaching(vmm::identAddress<uint64_t>(bar_phys_)/PAGE_SIZE, false);
  bar_ = vmm::identAddress<NVMeBar0 *>(bar_phys_);

  dbg::printf("nvme cap {}\n", bar_->capabilities_);
  dbg::printf("nvme version {}\n", bar_->version_);

  max_queue_ent_ = bar_->capabilities_ & 0xffffU;
  min_page_size_ = 1ULL<<(12+((bar_->capabilities_>>48)&0xf));
  max_page_size_ = 1ULL<<(12+((bar_->capabilities_>>52)&0xf));
  stride_ = 1ULL<<(2+((bar_->capabilities_>>32)&0xf));
  timeout_ = 500*((bar_->capabilities_>>24)&0xff);

}
