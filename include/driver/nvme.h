#pragma once
#include "stdint.h"

class NVMe {

  struct Bar0 {
    uint64_t capabilities_;
    uint32_t version_;
    uint32_t int_mask_set_;
    uint32_t int_mask_clear_;
    uint32_t controller_conf_;
    uint32_t unused1_;
    uint32_t controller_status_;
    uint32_t nvm_reset_;
    uint32_t admin_queue_attr_;
    uint64_t admin_sub_queue_;
    uint64_t admin_comp_queue_;
  };

  static_assert(sizeof(Bar0) == 0x38, "NVMeBar0 is not the correct size");

  struct AdminSubmission {
    uint32_t command_;
    uint32_t namespace_;
    uint32_t cdw2_;
    uint32_t cdw3_;
    uint64_t metadata_ptr_;
    uint64_t data_ptr1_;
    uint64_t data_ptr2_;
    uint32_t cdw10_;
    uint32_t cdw11_;
    uint32_t cdw12_;
    uint32_t cdw13_;
    uint32_t cdw14_;
    uint32_t cdw15_;
  };

  static_assert(sizeof(AdminSubmission) == 64, "AdminSubmission is not the correct size");

  struct AdminCompletion {
    uint32_t cdw0_;
    uint32_t cdw1_;
    uint16_t sq_head_ptr_;
    uint16_t sq_id_;
    uint16_t command_id_;
    uint16_t phase_tag_ : 1;
    uint16_t status_field_ : 15;
  };

  static_assert(sizeof(AdminCompletion) == 16, "AdminCompletion is not the correct size");

  uint8_t bus_;
  uint8_t dev_;
  uint32_t max_queue_ent_;
  uint64_t min_page_size_;
  uint64_t max_page_size_;
  uint64_t stride_;
  uint64_t timeout_; //ms
  uint32_t admin_c_head_;
  uint32_t admin_s_tail_;
  uint64_t bar_phys_;
  Bar0 *bar_;
  uint16_t *doorbells_;

  AdminSubmission *admin_sub_queue_;
  AdminCompletion *admin_comp_queue_;

public:
  NVMe(uint8_t bus, uint8_t dev);

  void sendAdminCommand(AdminSubmission sub);
};

