module;
#include <cstdint>

export module fat32;
import vfs;
import block;

export namespace fat32 {

struct BPB {
  uint8_t  jmp[3];
  char     oem[8];
  uint16_t bytes_per_sector;
  uint8_t  sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t  fat_count;
  uint16_t root_entry_count;
  uint16_t total_sectors_16;
  uint8_t  media_type;
  uint16_t sectors_per_fat_16;
  uint16_t sectors_per_track;
  uint16_t head_count;
  uint32_t hidden_sectors;
  uint32_t total_sectors_32;

  // FAT32 Extended BPB
  uint32_t sectors_per_fat_32;
  uint16_t ext_flags;
  uint16_t fs_version;
  uint32_t root_cluster;
  uint16_t fs_info;
  uint16_t backup_boot_sector;
  uint8_t  reserved[12];
  uint8_t  drive_number;
  uint8_t  reserved1;
  uint8_t  boot_signature;
  uint32_t volume_id;
  char     volume_label[11];
  char     fs_type[8];
} __attribute__((packed));

struct FATDirEntry {
  char     name[11];
  uint8_t  attr;
  uint8_t  nt_res;
  uint8_t  crt_time_tenth;
  uint16_t crt_time;
  uint16_t crt_date;
  uint16_t lst_acc_date;
  uint16_t fst_clus_hi;
  uint16_t wrt_time;
  uint16_t wrt_date;
  uint16_t fst_clus_lo;
  uint32_t file_size;
} __attribute__((packed));

class FAT32FileSystem;

class FAT32Node : public vfs::VfsNode {
public:
  FAT32Node(FAT32FileSystem *fs, FATDirEntry entry, uint32_t cluster, const char *name, uint64_t dir_sector = 0, uint32_t dir_offset = 0);
  
  int read(uint64_t offset, uint32_t size, void *buffer) override;
  int write(uint64_t offset, uint32_t size, const void *buffer) override;
  int readdir(uint32_t index, vfs::DirectoryEntry &entry) override;
  vfs::VfsNode *finddir(const char *name) override;
  uint64_t getSize() const override;
  vfs::NodeType getType() const override;

private:
  void updateDirectoryEntry();

  FAT32FileSystem *fs_;
  FATDirEntry entry_;
  uint32_t first_cluster_;
  char name_[256];
  uint64_t dir_sector_;
  uint32_t dir_offset_;
};

class FAT32FileSystem : public vfs::FileSystem {
public:
  FAT32FileSystem(BlockDevice *dev, uint64_t partition_start_lba);
  bool init();
  
  vfs::VfsNode *getRoot() override { return root_node_; }

  int readCluster(uint32_t cluster, void *buffer);
  int writeCluster(uint32_t cluster, const void *buffer);
  uint32_t getNextCluster(uint32_t cluster);
  uint32_t allocateCluster();
  int setNextCluster(uint32_t cluster, uint32_t next);

  BlockDevice *getDevice() { return dev_; }
  uint64_t getPartitionStartLBA() const { return partition_start_lba_; }
  uint32_t getSectorsPerCluster() const { return bpb_.sectors_per_cluster; }
  uint32_t getBytesPerSector() const { return bpb_.bytes_per_sector; }
  uint64_t getFirstDataSector() const { return first_data_sector_; }

private:
  BlockDevice *dev_;
  uint64_t partition_start_lba_;
  BPB bpb_;
  uint64_t first_fat_sector_;
  uint64_t first_data_sector_;
  uint32_t cluster_size_bytes_;
  vfs::VfsNode *root_node_;
};

} // namespace fat32
