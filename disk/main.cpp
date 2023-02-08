#include <fcntl.h>
#include <cassert>
#include <cstdint>
#include <unistd.h>
#include <cstdio>

struct Disk {
  int fd;

  struct PartitionTableEntry {
    uint8_t attributes_;
    uint8_t partition_start_chs_[3];
    uint8_t parition_type_;
    uint8_t last_sector_chs_[3];
    uint32_t partition_start_lba_;
    uint32_t number_of_sections_;
  } __attribute__((packed));

  struct MBR {
    uint8_t bootstrap_[440];
    uint32_t disk_id_;
    uint16_t reserved_;
    PartitionTableEntry parition_table_[4];
    uint16_t signature_;
  } __attribute__((packed)) mbr_;

  static_assert(sizeof(MBR) == 0x200);

  int parseMBR() {
    pread(fd, &mbr_, 0x200, 0);

    printf("%x\n", mbr_.signature_);

    return 0;
  }

};

int main() {
  Disk disk;
  disk.fd = open("disk.img", O_RDWR);
  assert(disk.fd != -1);
  disk.parseMBR();
}
