module;
#include <cstdint>

export module nvme;
import block;
import utility;

export class NVMe : public BlockDevice {
private:
  struct Completion;
  struct Submission;

public:
  NVMe(uint8_t bus, uint8_t dev);

  void sendAdminCommand(Submission sub);

  int readBlocks(uint64_t lba, uint32_t count, void *buffer) override;
  int writeBlocks(uint64_t lba, uint32_t count, const void *buffer) override;
  uint32_t getBlockSize() const override { return 512; }
  uint64_t getBlockCount() const override { return 1048576; } // 512 MB disk.img has 1048576 blocks

private:
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

  struct Submission {
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

  static_assert(sizeof(Submission) == 64, "AdminSubmission is not the correct size");

  struct Completion {
    uint32_t cdw0_;
    uint32_t cdw1_;
    uint16_t sq_head_ptr_;
    uint16_t sq_id_;
    uint16_t command_id_;
    uint16_t phase_tag_ : 1;
    uint16_t status_field_ : 15;
  };

  static_assert(sizeof(Completion) == 16, "AdminCompletion is not the correct size");
  struct Queue {
    Submission *sub_;
    Completion *comp_;
    uint16_t *doorbell_;
    uint32_t size_ = 0;
    uint32_t c_head_ = 0;
    uint32_t s_tail_ = 0;
    bool sendCommand(Submission sub);
    pair<bool, Completion> popResult();
  };

  uint8_t bus_;
  uint8_t dev_;
  uint32_t max_queue_ent_;
  uint64_t min_page_size_;
  uint64_t max_page_size_;
  uint64_t stride_;
  uint64_t timeout_; //ms
  uint64_t bar_phys_;
  Bar0 *bar_;
  uint16_t *doorbells_;

  Queue admin_queue_;
  Queue io_queue_;

  enum AdminCommands {
    DEL_IO_SUB = 0,
    CRE_IO_SUB,
    GET_LOG,
    DEL_IO_COM = 4,
    CRE_IO_COM,
    IDENT,
    ABORT = 8,
    SET_FEAT,
    GET_FEAT,
    ASYNC_REQ = 0xc,
    FIRMWARE_ACTIVE = 0x10,
    FIRMWARE_DOWNLOAD = 0x11
  };
};
