#pragma once
#include "stdint.h"

class BlockDevice {
public:
  virtual ~BlockDevice() = default;
  virtual int readBlocks(uint64_t lba, uint32_t count, void *buffer) = 0;
  virtual int writeBlocks(uint64_t lba, uint32_t count, const void *buffer) = 0;
  virtual uint32_t getBlockSize() const = 0;
  virtual uint64_t getBlockCount() const = 0;
};

namespace block {
  void registerDevice(BlockDevice *dev);
  BlockDevice *getDevice(uint32_t index);
  uint32_t getDeviceCount();
}
