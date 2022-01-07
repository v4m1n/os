#pragma once
#include "stdint.h"

class NVMe {

  struct NVMeBar0 {
    uint64_t capabilities_;
    uint32_t version_;
    uint32_t int_mask_set_;
    uint32_t int_mask_clear_;
    uint32_t controller_conf_;
    uint32_t unused1_;
    uint32_t controller_status_;
    uint32_t unused2_;
    uint32_t admin_queue_attr_;
    uint64_t admin_sub_queue_;
    uint64_t admin_comp_queue_;
  };

  uint8_t bus_;
  uint8_t dev_;
  uint32_t max_queue_ent_;
  uint64_t min_page_size_;
  uint64_t max_page_size_;
  uint64_t stride_;
  uint64_t timeout_; //ms
  uint64_t bar_phys_;
  NVMeBar0 *bar_;

public:
  NVMe(uint8_t bus, uint8_t dev);

};

