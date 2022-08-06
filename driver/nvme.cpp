#include "pci.h"
#include "nvme.h"
#include "debug.h"
#include "vmm.h"
#include "interrupts.h"

NVMe::NVMe(uint8_t bus, uint8_t dev) : bus_(bus), dev_(dev) {
  pci::writePCIConfig<uint16_t>(bus, dev, 0, 0x4, (1U<<4) | (1U<<2) | (1U<<1) | 1U);
  uint64_t bar1 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x10);
  uint64_t bar2 = pci::readPCIConfig<uint32_t>(bus, dev, 0, 0x18);
  dbg::panic_assert(bar1 != -1U && bar2 != -1U, "nvme device does not exist ({}, {})\n", bus, dev);
  bar_phys_ = (bar1 & ~0xfU) | bar2<<32;
  dbg::panic_assert(bar_phys_&~(PAGE_SIZE-1), "nvme device mmio not page aligned {}\n", bar_phys_);
  dbg::printf("nvme mmio {}\n", bar_phys_);
  bar_ = vmm::identUCAddress<Bar0 *>(bar_phys_);
  doorbells_ = vmm::identUCAddress<uint16_t *>(bar_phys_+0x1000);
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

  admin_queue_.sub_ = vmm::identUCAddress<Submission *>(bar_->admin_sub_queue_);
  admin_queue_.comp_ = vmm::identUCAddress<Completion *>(bar_->admin_comp_queue_);
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

  Queue io_queue;
  io_queue.size_ = 64;
  io_queue.doorbell_ = doorbells_+4;

  Submission sub;
  sub.command_ = 5;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  io_queue.comp_ = vmm::identUCAddress<Completion *>(sub.data_ptr1_);
  sub.cdw10_ = (63<<16)|1;
  sub.cdw11_ = 1;
  admin_queue_.sendCommand(sub);
  pair<bool, NVMe::Completion> tmp;
  while(!(tmp = admin_queue_.popResult()).first);
  dbg::panic_assert(tmp.second.status_field_ == 0, "io completion queue create error\n");
  
  sub.command_ = 1;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  io_queue.sub_ = vmm::identUCAddress<Submission *>(sub.data_ptr1_);
  sub.cdw10_ = (63<<16)|1;
  sub.cdw11_ = (1<<16)|1;
  admin_queue_.sendCommand(sub);
  while(!(tmp = admin_queue_.popResult()).first);
  dbg::panic_assert(tmp.second.status_field_ == 0, "io submission queue create error\n");


  sub.command_ = 2;
  sub.data_ptr1_ = pmm::allocZeroPFN()*PAGE_SIZE;
  vmm::identUCAddress<uint64_t *>(sub.data_ptr1_)[0] = pmm::allocZeroPFN()*PAGE_SIZE;
  sub.data_ptr2_ = 0;
  sub.metadata_ptr_ = pmm::allocZeroPFN()*PAGE_SIZE;
  sub.cdw10_ = 0x10000; //LBA
  sub.cdw11_ = 0; //LBA
  sub.cdw12_ = 0; //size
  sub.cdw13_ = 0;
  sub.cdw14_ = 0;
  sub.cdw15_ = 0;
  io_queue.sendCommand(sub);
  while(!(tmp = io_queue.popResult()).first);
  dbg::panic_assert(tmp.second.status_field_ == 0, "io read error {}\n", tmp.second.status_field_);

  dbg::panic("done\n");
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




