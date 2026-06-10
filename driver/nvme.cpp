module;
#include "stdint.h"
#include "stddef.h"

module nvme;
import cpu;
import string;
import debug;
import vmm;
import pmm;
import pci;
import interrupts;


NVMe::NVMe(uint8_t bus, uint8_t dev) : bus_(bus), dev_(dev) {
  pci::writePCIConfig<uint16_t>(bus, dev, 0, 0x4, (1U<<4) | (1U<<2) | (1U<<1) | 1U);
  uint64_t bar1 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x10);
  uint64_t bar2 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x14);
  dbg::panic_assert(bar1 != -1U && bar2 != -1U, "nvme device does not exist ({}, {})\n", bus, dev);
  bar_phys_ = (bar1 & ~0xfU) | bar2<<32;
  dbg::panic_assert(bar_phys_&~(PAGE_SIZE-1), "nvme device mmio not page aligned {}\n", bar_phys_);
  dbg::printf("nvme mmio {}\n", bar_phys_);
  bar_ = (Bar0 *)vmm::AddressSpace::ioremap(bar_phys_/PAGE_SIZE);
  doorbells_ = (uint16_t *)vmm::AddressSpace::ioremap((bar_phys_+0x1000)/PAGE_SIZE);
  //bar_->nvm_reset_ = 0x4E564D65U;

  bar_->controller_conf_ &= ~1ULL;
  while((atomic_fetch(bar_->controller_status_)&1)) asm volatile("pause" ::: "memory");
  dbg::printf("nvme cap {}\n", bar_->capabilities_);
  dbg::printf("nvme version {}\n", bar_->version_);

  max_queue_ent_ = bar_->capabilities_ & 0xffffU;
  min_page_size_ = 1ULL<<(12+((bar_->capabilities_>>48)&0xf));
  max_page_size_ = 1ULL<<(12+((bar_->capabilities_>>52)&0xf));
  stride_ = 1ULL<<(2+((bar_->capabilities_>>32)&0xf));
  timeout_ = 500*((bar_->capabilities_>>24)&0xff);

  //bar_->controller_conf_ = ((bar_->capabilities_>>48)&0xf)<<7;


  dbg::panic_assert(min_page_size_ == PAGE_SIZE, "min_page_size_ not page size\n");
  bar_->admin_sub_queue_ = pmm::allocZeroPFN()*PAGE_SIZE;
  bar_->admin_comp_queue_ = pmm::allocZeroPFN()*PAGE_SIZE;

  admin_queue_.sub_ = vmm::identAddress<Submission *>(bar_->admin_sub_queue_);
  admin_queue_.comp_ = vmm::identAddress<Completion *>(bar_->admin_comp_queue_);
  admin_queue_.doorbell_ = doorbells_;
  admin_queue_.size_ = 64;


  bar_->admin_queue_attr_ = (63)|((63)<<16);
  bar_->controller_conf_ = 0<<4;
  bar_->controller_conf_ |= 6 << 16;
  bar_->controller_conf_ |= 4 << 20;
  bar_->controller_conf_ |= 1;

  while(!(atomic_fetch(bar_->controller_status_)&1)) asm volatile("pause" ::: "memory");
  dbg::printf("nvme device ready\n");
  dbg::printf("nvme stride {}\n", stride_);

  io_queue_.size_ = 64;
  io_queue_.doorbell_ = doorbells_+4;

  Submission sub;
  sub.command_ = CRE_IO_COM;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  io_queue_.comp_ = vmm::identAddress<Completion *>(sub.data_ptr1_);
  sub.cdw10_ = (63<<16)|1;
  sub.cdw11_ = 1;
  admin_queue_.sendCommand(sub);
  pair<bool, NVMe::Completion> tmp;
  while(!(tmp = admin_queue_.popResult()).first);
  dbg::panic_assert(tmp.second.status_field_ == 0, "io completion queue create error\n");
  
  sub.command_ = CRE_IO_SUB;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  io_queue_.sub_ = vmm::identAddress<Submission *>(sub.data_ptr1_);
  sub.cdw10_ = (63<<16)|1;
  sub.cdw11_ = (1<<16)|1;
  admin_queue_.sendCommand(sub);
  while(!(tmp = admin_queue_.popResult()).first);
  dbg::panic_assert(tmp.second.status_field_ == 0, "io submission queue create error\n");

  dbg::printf("nvme drive fully initialized and ready\n");
}

int NVMe::readBlocks(uint64_t lba, uint32_t count, void *buffer) {
  char *ptr = reinterpret_cast<char *>(buffer);
  for (uint32_t i = 0; i < count; ++i) {
    size_t pfn = pmm::allocZeroPFN();
    uint64_t phys_addr = pfn * PAGE_SIZE;
    void *virt_addr = vmm::identAddress<void *>(phys_addr);

    Submission sub{};
    sub.command_ = 0x02; // Read
    sub.namespace_ = 1;
    sub.data_ptr1_ = phys_addr;
    sub.data_ptr2_ = 0;
    sub.cdw10_ = static_cast<uint32_t>((lba + i) & 0xFFFFFFFF);
    sub.cdw11_ = static_cast<uint32_t>((lba + i) >> 32);
    sub.cdw12_ = 0; // 1 block

    while (!io_queue_.sendCommand(sub)) {
      asm volatile("pause" ::: "memory");
    }

    pair<bool, Completion> res;
    while (!(res = io_queue_.popResult()).first) {
      asm volatile("pause" ::: "memory");
    }

    if (res.second.status_field_ != 0) {
      pmm::freePFN(pfn);
      return -1;
    }

    memcpy(ptr + i * 512, virt_addr, 512);
    pmm::freePFN(pfn);
  }
  return 0;
}

int NVMe::writeBlocks(uint64_t lba, uint32_t count, const void *buffer) {
  const char *ptr = reinterpret_cast<const char *>(buffer);
  for (uint32_t i = 0; i < count; ++i) {
    size_t pfn = pmm::allocZeroPFN();
    uint64_t phys_addr = pfn * PAGE_SIZE;
    void *virt_addr = vmm::identAddress<void *>(phys_addr);

    memcpy(virt_addr, ptr + i * 512, 512);

    Submission sub{};
    sub.command_ = 0x01; // Write
    sub.namespace_ = 1;
    sub.data_ptr1_ = phys_addr;
    sub.data_ptr2_ = 0;
    sub.cdw10_ = static_cast<uint32_t>((lba + i) & 0xFFFFFFFF);
    sub.cdw11_ = static_cast<uint32_t>((lba + i) >> 32);
    sub.cdw12_ = 0; // 1 block

    while (!io_queue_.sendCommand(sub)) {
      asm volatile("pause" ::: "memory");
    }

    pair<bool, Completion> res;
    while (!(res = io_queue_.popResult()).first) {
      asm volatile("pause" ::: "memory");
    }

    if (res.second.status_field_ != 0) {
      pmm::freePFN(pfn);
      return -1;
    }

    pmm::freePFN(pfn);
  }
  return 0;
}

void createIOQueue() {

}

bool NVMe::Queue::sendCommand(Submission sub) {
  if (((s_tail_ + 1)%size_) == c_head_) return false;
  sub_[s_tail_] = sub;
  s_tail_ = (s_tail_ + 1)%size_;
  *doorbell_ = s_tail_;
  return true;
}
pair<bool, NVMe::Completion> NVMe::Queue::popResult() {
  if(comp_[c_head_].phase_tag_ == 0) return {false, {}};
  auto ret = comp_[c_head_];
  comp_[c_head_].phase_tag_ = 0;
  c_head_ = (c_head_ + 1)%size_;
  *(doorbell_+1) = c_head_;
  return {true, ret};
}




