module;
#include <cstdint>

module fat32;
import knew;
import string;
import kmm;
import debug;


namespace fat32 {

static void parseFATName(const char *fat_name, char *out_name) {
  int name_len = 8;
  while (name_len > 0 && fat_name[name_len - 1] == ' ') {
    name_len--;
  }
  int ext_len = 3;
  while (ext_len > 0 && fat_name[8 + ext_len - 1] == ' ') {
    ext_len--;
  }

  int out_idx = 0;
  for (int i = 0; i < name_len; ++i) {
    char c = fat_name[i];
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    out_name[out_idx++] = c;
  }
  if (ext_len > 0) {
    out_name[out_idx++] = '.';
    for (int i = 0; i < ext_len; ++i) {
      char c = fat_name[8 + i];
      if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
      out_name[out_idx++] = c;
    }
  }
  out_name[out_idx] = '\0';
}

FAT32Node::FAT32Node(FAT32FileSystem *fs, FATDirEntry entry, uint32_t cluster, const char *name, uint64_t dir_sector, uint32_t dir_offset)
    : fs_(fs), entry_(entry), first_cluster_(cluster), dir_sector_(dir_sector), dir_offset_(dir_offset) {
  strncpy(name_, name, sizeof(name_));
  name_[sizeof(name_) - 1] = '\0';
}

int64_t FAT32Node::read(uint64_t offset, uint32_t size, void *buffer) {
  uint64_t file_size = getSize();
  if (offset >= file_size) return 0;
  if (offset + size > file_size) {
    size = file_size - offset;
  }

  uint32_t cluster_size = fs_->getSectorsPerCluster() * fs_->getBytesPerSector();
  uint32_t curr_cluster = first_cluster_;
  
  uint64_t skip_clusters = offset / cluster_size;
  for (uint64_t i = 0; i < skip_clusters; ++i) {
    curr_cluster = fs_->getNextCluster(curr_cluster);
    if (curr_cluster >= 0x0FFFFFF8 || curr_cluster == 0) {
      return -1;
    }
  }

  uint32_t bytes_read = 0;
  uint64_t start_offset = offset % cluster_size;
  
  char *cluster_buf = reinterpret_cast<char *>(kmm::kmalloc(cluster_size));
  if (!cluster_buf) return -1;

  while (bytes_read < size) {
    if (fs_->readCluster(curr_cluster, cluster_buf) != 0) {
      kmm::kfree(cluster_buf);
      return -1;
    }

    uint32_t to_copy = (cluster_size - start_offset < size - bytes_read) ? 
                        (cluster_size - start_offset) : (size - bytes_read);
    memcpy(reinterpret_cast<char *>(buffer) + bytes_read, cluster_buf + start_offset, to_copy);
    bytes_read += to_copy;
    
    start_offset = 0;

    if (bytes_read < size) {
      curr_cluster = fs_->getNextCluster(curr_cluster);
      if (curr_cluster >= 0x0FFFFFF8 || curr_cluster == 0) {
        break;
      }
    }
  }

  kmm::kfree(cluster_buf);
  return bytes_read;
}

void FAT32Node::updateDirectoryEntry() {
  if (dir_sector_ == 0) return;

  char *sector_buf = reinterpret_cast<char *>(kmm::kmalloc(fs_->getBytesPerSector()));
  if (!sector_buf) return;

  if (fs_->getDevice()->readBlocks(dir_sector_, 1, sector_buf) != 0) {
    kmm::kfree(sector_buf);
    return;
  }

  entry_.fst_clus_lo = first_cluster_ & 0xFFFF;
  entry_.fst_clus_hi = (first_cluster_ >> 16) & 0xFFFF;

  memcpy(sector_buf + dir_offset_, &entry_, sizeof(FATDirEntry));

  if (fs_->getDevice()->writeBlocks(dir_sector_, 1, sector_buf) != 0) {
    kmm::kfree(sector_buf);
    return;
  }

  kmm::kfree(sector_buf);
}

int64_t FAT32Node::write(uint64_t offset, uint32_t size, const void *buffer) {
  uint32_t cluster_size = fs_->getSectorsPerCluster() * fs_->getBytesPerSector();
  uint64_t needed_end = offset + size;
  uint64_t current_allocated_bytes = 0;
  
  if (first_cluster_ != 0) {
    uint32_t curr = first_cluster_;
    uint32_t count = 1;
    while (true) {
      uint32_t next = fs_->getNextCluster(curr);
      if (next >= 0x0FFFFFF8 || next == 0) {
        break;
      }
      curr = next;
      count++;
    }
    current_allocated_bytes = (uint64_t)count * cluster_size;
  }

  if (needed_end > current_allocated_bytes) {
    uint64_t bytes_to_alloc = needed_end - current_allocated_bytes;
    uint32_t clusters_to_alloc = (bytes_to_alloc + cluster_size - 1) / cluster_size;

    for (uint32_t i = 0; i < clusters_to_alloc; ++i) {
      uint32_t new_cluster = fs_->allocateCluster();
      if (new_cluster == 0) {
        return -1;
      }

      if (first_cluster_ == 0) {
        first_cluster_ = new_cluster;
        updateDirectoryEntry();
      } else {
        uint32_t curr = first_cluster_;
        while (true) {
          uint32_t next = fs_->getNextCluster(curr);
          if (next >= 0x0FFFFFF8 || next == 0) {
            break;
          }
          curr = next;
        }
        if (fs_->setNextCluster(curr, new_cluster) != 0) {
          return -1;
        }
      }
    }
  }

  uint32_t curr_cluster = first_cluster_;
  uint64_t skip_clusters = offset / cluster_size;
  for (uint64_t i = 0; i < skip_clusters; ++i) {
    curr_cluster = fs_->getNextCluster(curr_cluster);
    if (curr_cluster >= 0x0FFFFFF8 || curr_cluster == 0) {
      return -1;
    }
  }

  uint32_t bytes_written = 0;
  uint64_t start_offset = offset % cluster_size;

  char *cluster_buf = reinterpret_cast<char *>(kmm::kmalloc(cluster_size));
  if (!cluster_buf) return -1;

  while (bytes_written < size) {
    if (start_offset > 0 || (size - bytes_written) < cluster_size) {
      if (fs_->readCluster(curr_cluster, cluster_buf) != 0) {
        kmm::kfree(cluster_buf);
        return -1;
      }
    }

    uint32_t to_copy = (cluster_size - start_offset < size - bytes_written) ?
                        (cluster_size - start_offset) : (size - bytes_written);
    memcpy(cluster_buf + start_offset, reinterpret_cast<const char *>(buffer) + bytes_written, to_copy);

    if (fs_->writeCluster(curr_cluster, cluster_buf) != 0) {
      kmm::kfree(cluster_buf);
      return -1;
    }

    bytes_written += to_copy;
    start_offset = 0;

    if (bytes_written < size) {
      curr_cluster = fs_->getNextCluster(curr_cluster);
      if (curr_cluster >= 0x0FFFFFF8 || curr_cluster == 0) {
        break;
      }
    }
  }

  kmm::kfree(cluster_buf);

  if (needed_end > entry_.file_size) {
    entry_.file_size = needed_end;
    updateDirectoryEntry();
  }

  return bytes_written;
}

int FAT32Node::readdir(uint32_t index, vfs::DirectoryEntry &entry) {
  if (getType() != vfs::NodeType::DIRECTORY) return -1;

  uint32_t cluster_size = fs_->getSectorsPerCluster() * fs_->getBytesPerSector();
  uint32_t curr_cluster = first_cluster_;

  char *cluster_buf = reinterpret_cast<char *>(kmm::kmalloc(cluster_size));
  if (!cluster_buf) return -1;

  uint32_t current_index = 0;
  bool finished = false;

  while (curr_cluster < 0x0FFFFFF8 && curr_cluster != 0 && !finished) {
    if (fs_->readCluster(curr_cluster, cluster_buf) != 0) {
      kmm::kfree(cluster_buf);
      return -1;
    }

    for (uint32_t offset = 0; offset < cluster_size; offset += sizeof(FATDirEntry)) {
      auto *fat_entry = reinterpret_cast<FATDirEntry *>(cluster_buf + offset);
      
      if (fat_entry->name[0] == 0x00) {
        finished = true;
        break;
      }
      if (static_cast<uint8_t>(fat_entry->name[0]) == 0xE5) {
        continue;
      }
      if (fat_entry->attr == 0x0F) {
        continue;
      }
      if (fat_entry->attr & 0x08) {
        continue;
      }

      if (current_index == index) {
        parseFATName(fat_entry->name, entry.name);
        entry.inode = (static_cast<uint32_t>(fat_entry->fst_clus_hi) << 16) | fat_entry->fst_clus_lo;
        entry.type = (fat_entry->attr & 0x10) ? vfs::NodeType::DIRECTORY : vfs::NodeType::FILE;
        kmm::kfree(cluster_buf);
        return 0;
      }
      current_index++;
    }

    if (!finished) {
      curr_cluster = fs_->getNextCluster(curr_cluster);
    }
  }

  kmm::kfree(cluster_buf);
  return -1;
}

vfs::VfsNode *FAT32Node::finddir(const char *name) {
  if (getType() != vfs::NodeType::DIRECTORY) return nullptr;

  uint32_t cluster_size = fs_->getSectorsPerCluster() * fs_->getBytesPerSector();
  uint32_t curr_cluster = first_cluster_;

  char *cluster_buf = reinterpret_cast<char *>(kmm::kmalloc(cluster_size));
  if (!cluster_buf) return nullptr;

  bool finished = false;

  while (curr_cluster < 0x0FFFFFF8 && curr_cluster != 0 && !finished) {
    if (fs_->readCluster(curr_cluster, cluster_buf) != 0) {
      kmm::kfree(cluster_buf);
      return nullptr;
    }

    for (uint32_t offset = 0; offset < cluster_size; offset += sizeof(FATDirEntry)) {
      auto *fat_entry = reinterpret_cast<FATDirEntry *>(cluster_buf + offset);
      
      if (fat_entry->name[0] == 0x00) {
        finished = true;
        break;
      }
      if (static_cast<uint8_t>(fat_entry->name[0]) == 0xE5) {
        continue;
      }
      if (fat_entry->attr == 0x0F) {
        continue;
      }
      if (fat_entry->attr & 0x08) {
        continue;
      }

      char parsed_name[256];
      parseFATName(fat_entry->name, parsed_name);

      if (strcmp(parsed_name, name) == 0) {
        uint32_t cluster = (static_cast<uint32_t>(fat_entry->fst_clus_hi) << 16) | fat_entry->fst_clus_lo;
        uint64_t entry_sector = fs_->getFirstDataSector() + (curr_cluster - 2) * fs_->getSectorsPerCluster() + (offset / fs_->getBytesPerSector());
        uint32_t entry_offset = offset % fs_->getBytesPerSector();

        void *mem = kmm::kmalloc(sizeof(FAT32Node));
        if (!mem) {
          kmm::kfree(cluster_buf);
          return nullptr;
        }
        auto *node = new (mem) FAT32Node(fs_, *fat_entry, cluster, parsed_name, entry_sector, entry_offset);
        kmm::kfree(cluster_buf);
        return node;
      }
    }

    if (!finished) {
      curr_cluster = fs_->getNextCluster(curr_cluster);
    }
  }

  kmm::kfree(cluster_buf);
  return nullptr;
}

uint64_t FAT32Node::getSize() const {
  return entry_.file_size;
}

vfs::NodeType FAT32Node::getType() const {
  return (entry_.attr & 0x10) ? vfs::NodeType::DIRECTORY : vfs::NodeType::FILE;
}

FAT32FileSystem::FAT32FileSystem(BlockDevice *dev, uint64_t partition_start_lba)
    : dev_(dev), partition_start_lba_(partition_start_lba), root_node_(nullptr) {}

bool FAT32FileSystem::init() {
  uint32_t sec_size = dev_->getBlockSize();
  char *sector0 = reinterpret_cast<char *>(kmm::kmalloc(sec_size));
  if (!sector0) return false;

  if (dev_->readBlocks(partition_start_lba_, 1, sector0) != 0) {
    kmm::kfree(sector0);
    return false;
  }

  memcpy(&bpb_, sector0, sizeof(BPB));
  kmm::kfree(sector0);

  // Quick sanity checks
  if (bpb_.bytes_per_sector != 512 || bpb_.sectors_per_cluster == 0) {
    dbg::printf("FAT32: invalid sector size %d or sectors/cluster %d\n", 
                bpb_.bytes_per_sector, bpb_.sectors_per_cluster);
    return false;
  }

  first_fat_sector_ = partition_start_lba_ + bpb_.reserved_sectors;
  first_data_sector_ = first_fat_sector_ + (bpb_.fat_count * bpb_.sectors_per_fat_32);
  cluster_size_bytes_ = bpb_.sectors_per_cluster * bpb_.bytes_per_sector;

  FATDirEntry root_entry{};
  root_entry.attr = 0x10; // Directory
  root_entry.file_size = 0;
  root_entry.fst_clus_lo = bpb_.root_cluster & 0xFFFF;
  root_entry.fst_clus_hi = (bpb_.root_cluster >> 16) & 0xFFFF;

  void *mem = kmm::kmalloc(sizeof(FAT32Node));
  if (!mem) return false;
  root_node_ = new (mem) FAT32Node(this, root_entry, bpb_.root_cluster, "/");
  return true;
}

int FAT32FileSystem::readCluster(uint32_t cluster, void *buffer) {
  uint64_t sector = first_data_sector_ + (cluster - 2) * bpb_.sectors_per_cluster;
  return dev_->readBlocks(sector, bpb_.sectors_per_cluster, buffer);
}

uint32_t FAT32FileSystem::getNextCluster(uint32_t cluster) {
  uint32_t fat_sec = first_fat_sector_ + (cluster * 4) / bpb_.bytes_per_sector;
  uint32_t fat_off = (cluster * 4) % bpb_.bytes_per_sector;

  char *sec_buf = reinterpret_cast<char *>(kmm::kmalloc(bpb_.bytes_per_sector));
  if (!sec_buf) return 0x0FFFFFFF;

  if (dev_->readBlocks(fat_sec, 1, sec_buf) != 0) {
    kmm::kfree(sec_buf);
    return 0x0FFFFFFF;
  }

  uint32_t next_cluster = *reinterpret_cast<uint32_t *>(sec_buf + fat_off);
  kmm::kfree(sec_buf);

  return next_cluster & 0x0FFFFFFF;
}

int FAT32FileSystem::writeCluster(uint32_t cluster, const void *buffer) {
  uint64_t sector = first_data_sector_ + (cluster - 2) * bpb_.sectors_per_cluster;
  return dev_->writeBlocks(sector, bpb_.sectors_per_cluster, buffer);
}

uint32_t FAT32FileSystem::allocateCluster() {
  uint32_t total_clusters = bpb_.total_sectors_32 / bpb_.sectors_per_cluster;
  if (total_clusters == 0) {
    total_clusters = bpb_.total_sectors_16 / bpb_.sectors_per_cluster;
  }

  char *sec_buf = reinterpret_cast<char *>(kmm::kmalloc(bpb_.bytes_per_sector));
  if (!sec_buf) return 0;

  uint32_t current_sector = 0xFFFFFFFF;

  for (uint32_t cluster = 2; cluster < total_clusters; ++cluster) {
    uint32_t fat_sec = first_fat_sector_ + (cluster * 4) / bpb_.bytes_per_sector;
    uint32_t fat_off = (cluster * 4) % bpb_.bytes_per_sector;

    if (fat_sec != current_sector) {
      if (dev_->readBlocks(fat_sec, 1, sec_buf) != 0) {
        kmm::kfree(sec_buf);
        return 0;
      }
      current_sector = fat_sec;
    }

    uint32_t val = *reinterpret_cast<uint32_t *>(sec_buf + fat_off) & 0x0FFFFFFF;
    if (val == 0) {
      *reinterpret_cast<uint32_t *>(sec_buf + fat_off) = 0x0FFFFFFF | (*reinterpret_cast<uint32_t *>(sec_buf + fat_off) & 0xF0000000);
      if (dev_->writeBlocks(fat_sec, 1, sec_buf) != 0) {
        kmm::kfree(sec_buf);
        return 0;
      }
      kmm::kfree(sec_buf);
      return cluster;
    }
  }

  kmm::kfree(sec_buf);
  return 0;
}

int FAT32FileSystem::setNextCluster(uint32_t cluster, uint32_t next) {
  uint32_t fat_sec = first_fat_sector_ + (cluster * 4) / bpb_.bytes_per_sector;
  uint32_t fat_off = (cluster * 4) % bpb_.bytes_per_sector;

  char *sec_buf = reinterpret_cast<char *>(kmm::kmalloc(bpb_.bytes_per_sector));
  if (!sec_buf) return -1;

  if (dev_->readBlocks(fat_sec, 1, sec_buf) != 0) {
    kmm::kfree(sec_buf);
    return -1;
  }

  *reinterpret_cast<uint32_t *>(sec_buf + fat_off) = (next & 0x0FFFFFFF) | (*reinterpret_cast<uint32_t *>(sec_buf + fat_off) & 0xF0000000);

  if (dev_->writeBlocks(fat_sec, 1, sec_buf) != 0) {
    kmm::kfree(sec_buf);
    return -1;
  }

  kmm::kfree(sec_buf);
  return 0;
}

} // namespace fat32
