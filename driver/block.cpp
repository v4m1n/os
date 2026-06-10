#include "driver/block.h"

namespace block {
  constexpr uint32_t MAX_DEVICES = 16;
  static BlockDevice *devices[MAX_DEVICES];
  static uint32_t device_count = 0;

  void registerDevice(BlockDevice *dev) {
    if (device_count < MAX_DEVICES) {
      devices[device_count++] = dev;
    }
  }

  BlockDevice *getDevice(uint32_t index) {
    if (index < device_count) {
      return devices[index];
    }
    return nullptr;
  }

  uint32_t getDeviceCount() {
    return device_count;
  }
}
