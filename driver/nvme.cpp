#include "pci.h"
#include "nvme.h"
#include "debug.h"
#include "vmm.h"

NVMe::NVMe(uint8_t bus, uint8_t dev) : bus_(bus), dev_(dev) {
  pci::writePCIConfig<uint16_t>(bus, dev, 0, 0x4, 1U<<4 | 1U<<2 | 1U<<1);
  uint64_t bar1 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x10);
  uint64_t bar2 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x18);
  dbg::panic_assert(bar1 != -1U && bar2 != -1U, "nvme device does not exist ({}, {})\n", bus, dev);
  bar_phys_ = (bar1 & ~0xfU) | bar2<<32;
  dbg::panic_assert(bar_phys_&~(PAGE_SIZE-1), "nvme device mmio not page aligned {}\n", bar_phys_);
  dbg::printf("nvme mmio {}\n", bar_phys_);
  bar_ = vmm::identUCAddress<Bar0 *>(bar_phys_);
  doorbells_ = vmm::identUCAddress<uint16_t *>(bar_phys_+0x1000);
  bar_->nvm_reset_ = 0x4E564D65U;

  dbg::printf("nvme cap {}\n", bar_->capabilities_);
  dbg::printf("nvme version {}\n", bar_->version_);

  max_queue_ent_ = bar_->capabilities_ & 0xffffU;
  min_page_size_ = 1ULL<<(12+((bar_->capabilities_>>48)&0xf));
  max_page_size_ = 1ULL<<(12+((bar_->capabilities_>>52)&0xf));
  stride_ = 1ULL<<(2+((bar_->capabilities_>>32)&0xf));
  timeout_ = 500*((bar_->capabilities_>>24)&0xff);

  bar_->controller_conf_ = ((bar_->capabilities_>>48)&0xf)<<7;

  dbg::panic_assert(min_page_size_ == PAGE_SIZE, "min_page_size_ not page size\n");
  bar_->admin_sub_queue_ = pmm::allocZeroPFN()*PAGE_SIZE;
  bar_->admin_comp_queue_ = pmm::allocZeroPFN()*PAGE_SIZE;

  admin_sub_queue_ = vmm::identUCAddress<AdminSubmission *>(bar_->admin_sub_queue_);
  admin_comp_queue_ = vmm::identUCAddress<AdminCompletion *>(bar_->admin_comp_queue_);

  bar_->admin_queue_attr_ = 64|(64<<16);

  bar_->controller_conf_ |= 1;
  while(atomic_fetch(bar_->controller_status_)&1) asm volatile("pause" ::: "memory");
  dbg::printf("nvme device ready\n");

  admin_c_head_ = 0;
  admin_s_tail_ = 0;

  AdminSubmission sub;
  sub.command_ = 6;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  sub.cdw10_ = 2;
  sendAdminCommand(sub);
  while(1) dbg::printf("{d}", admin_comp_queue_->phase_tag_);
}


void NVMe::sendAdminCommand(AdminSubmission sub) {
  admin_sub_queue_[admin_s_tail_] = sub;
  admin_s_tail_ = (admin_s_tail_ + 1)%64;
  doorbells_[0] = admin_s_tail_; 
}
