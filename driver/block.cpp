module;
#include <cstdint>

module block;
import knew;
import fat32;
import vfs;
import string;
import kmm;
import debug;

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

  struct PartitionTableEntry {
    uint8_t attributes_;
    uint8_t partition_start_chs_[3];
    uint8_t partition_type_;
    uint8_t last_sector_chs_[3];
    uint32_t partition_start_lba_;
    uint32_t number_of_sections_;
  } __attribute__((packed));

  struct MBR {
    uint8_t bootstrap_[440];
    uint32_t disk_id_;
    uint16_t reserved_;
    PartitionTableEntry partition_table_[4];
    uint16_t signature_;
  } __attribute__((packed));

  void detectAndMountPartitions(BlockDevice *dev) {
    if (!dev) return;

    MBR mbr;
    if (dev->readBlocks(0, 1, &mbr) == 0 && mbr.signature_ == 0xAA55) {
      for (int i = 0; i < 4; ++i) {
        auto &entry = mbr.partition_table_[i];
        if (entry.partition_type_ != 0) {
          dbg::printf("Block: Found partition {}: type={}, start_lba={}\n",
                      (uint64_t)i, (uint64_t)entry.partition_type_, (uint64_t)entry.partition_start_lba_);
          
          void *fs_mem = kmm::kmalloc(sizeof(fat32::FAT32FileSystem));
          fat32::FAT32FileSystem *fs = nullptr;
          if (fs_mem) {
            fs = new (fs_mem) fat32::FAT32FileSystem(dev, entry.partition_start_lba_);
          }

          if (fs && fs->init()) {
            vfs::mount("/", fs);
            dbg::printf("Block: Mounted FAT32 filesystem from partition {} to /\n", (uint64_t)i);
            break; // Mount the first valid FAT32 partition as root
          } else {
            if (fs) {
              fs->~FAT32FileSystem();
              kmm::kfree(fs);
            } else if (fs_mem) {
              kmm::kfree(fs_mem);
            }
          }
        }
      }
    }
  }
}
