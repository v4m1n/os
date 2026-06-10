#include <fcntl.h>
#include <cassert>
#include <cstdint>
#include <unistd.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>

struct ExtExtentHeader {
  uint16_t eh_magic;      // 0xF30A
  uint16_t eh_entries;    // Number of valid entries
  uint16_t eh_max;        // Max number of entries
  uint16_t eh_depth;      // Depth of tree (0 for leaves, >0 for internal nodes)
  uint32_t eh_generation; // Generation of the tree
} __attribute__((packed));

struct ExtExtent {
  uint32_t ee_block;      // First logical block number
  uint16_t ee_len;        // Number of blocks covered
  uint16_t ee_start_hi;   // Upper 16-bits of physical block address
  uint32_t ee_start_lo;   // Lower 32-bits of physical block address
} __attribute__((packed));

struct ExtExtentIdx {
  uint32_t ei_block;      // Index covers logical blocks from this value upwards
  uint32_t ei_leaf_lo;    // Lower 32-bits of physical block of next level
  uint16_t ei_leaf_hi;    // Upper 16-bits of physical block of next level
  uint16_t ei_unused;
} __attribute__((packed));

struct Inode {
  uint16_t i_mode;        // File mode
  uint16_t i_uid;         // Low 16 bits of Owner Uid
  uint32_t i_size_lo;     // Size in bytes
  uint32_t i_atime;       // Access time
  uint32_t i_ctime;       // Creation time
  uint32_t i_mtime;       // Modification time
  uint32_t i_dtime;       // Deletion Time
  uint16_t i_gid;         // Low 16 bits of Group Id
  uint16_t i_links_count; // Links count
  uint32_t i_blocks_lo;   // Blocks count
  uint32_t i_flags;       // File flags
  union {
      struct {
          uint32_t l_i_version;
      } linux1;
      struct {
          uint32_t  h_i_translator;
      } hurd1;
      struct {
          uint32_t  m_i_reserved1;
      } masix1;
  } osd1;                 // OS dependent 1
  uint32_t i_block[15];   // Pointers to blocks / Extent tree
  uint32_t i_generation;  // File version (for NFS)
  uint32_t i_file_acl_lo; // File ACL
  uint32_t i_size_high;   // Size high
  uint32_t i_obso_faddr;  // Obsoleted fragment address
  union {
      struct {
          uint16_t l_i_blocks_high; // were l_i_reserved1
          uint16_t l_i_file_acl_high;
          uint16_t l_i_uid_high;    // these 2 fields
          uint16_t l_i_gid_high;    // were reserved2[2]
          uint16_t l_i_checksum_lo; // crc32c(uuid+inum+inode) LE
          uint16_t l_i_reserved;
      } linux2;
      struct {
          uint16_t h_i_reserved1;
          uint16_t h_i_mode_high;
          uint16_t h_i_uid_high;
          uint16_t h_i_gid_high;
          uint32_t h_i_author;
      } hurd2;
      struct {
          uint16_t m_i_reserved2[2];
          uint16_t m_i_pad1;
          uint16_t m_i_uid_high;    // these 2 fields
          uint16_t m_i_gid_high;    // were reserved2[2]
          uint32_t m_i_reserved3;
      } masix2;
  } osd2;                 // OS dependent 2
  uint16_t i_extra_isize;
  uint16_t i_checksum_hi; // crc32c(uuid+inum+inode) BE
  uint32_t i_ctime_extra; // extra Change time (nsec << 2 | epoch)
  uint32_t i_mtime_extra; // extra Modification time (nsec << 2 | epoch)
  uint32_t i_atime_extra; // extra Access time (nsec << 2 | epoch)
  uint32_t i_crtime;      // File Creation time
  uint32_t i_crtime_extra;// extra FileCreation time (nsec << 2 | epoch)
  uint32_t i_version_hi;  // high 32 bits for 64-bit version
  uint32_t i_projid;      // Project ID
} __attribute__((packed));

struct ExtDirEntry {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} __attribute__((packed));

uint32_t crctable[256] = {
 0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
 0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
 0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL,
 0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
 0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
 0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
 0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L,
 0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
 0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL,
 0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
 0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L,
 0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
 0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L,
 0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
 0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
 0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
 0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L,
 0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
 0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L,
 0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
 0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L,
 0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
 0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L,
 0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
 0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
 0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
 0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L,
 0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
 0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L,
 0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
 0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L,
 0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
 0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL,
 0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
 0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
 0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
 0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L,
 0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
 0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL,
 0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
 0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L,
 0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
 0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL,
 0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
 0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
 0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
 0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L,
 0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
 0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L,
 0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
 0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL,
 0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
 0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL,
 0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
 0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
 0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
 0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL,
 0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
 0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L,
 0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
 0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L,
 0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
 0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL,
 0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
};
struct CRC32 {
  uint32_t value_ = 0xffffffffULL;
  template <typename T>
  void push(T in) {
    static_assert(!std::is_pointer<T>::value);
  	uint8_t *buf = (uint8_t *)&in;
    size_t len = sizeof(T);
  	while (len-- > 0) {
  		value_ = (value_>>8) ^ crctable[(value_ ^ (*buf++)) & 0xFF];
  	}
  }
  void push(uint8_t *buf, size_t len) {
  	while (len-- > 0) {
  		value_ = (value_>>8) ^ crctable[(value_ ^ (*buf++)) & 0xFF];
  	}
  }
  uint32_t get() {return value_^0xffffffffU;}
};

uint32_t crc32c(uint8_t *buf, size_t len, uint32_t init = 0xffffffffULL)
{
	uint32_t crc = init;
	while (len-- > 0) {
		crc = (crc>>8) ^ crctable[(crc ^ (*buf++)) & 0xFF];
	}
	return crc^0xffffffff;
}

struct Disk;

struct Partition {
  Disk *disk_;
  uint64_t start_lba_;
  uint8_t uuid_[16];
  virtual int parseSuperblock() = 0;
  Partition(Disk *disk, uint64_t start_lba) : disk_(disk), start_lba_(start_lba){}
  
};


struct Ext4Partition : Partition{
  struct SuperBlock {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count_lo;
    uint32_t s_r_blocks_count_lo;
    uint32_t s_free_blocks_count_lo;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_cluster_size;
    uint32_t s_blocks_per_group;
    uint32_t s_clusters_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    //EXT4_DYNAMIC_REV
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algorithm_usage_bitmap;

    //EXT4_FEATURE_COMPAT_DIR_PREALLOC
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;

    //EXT4_FEATURE_COMPAT_HAS_JOURNAL
    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t s_jnl_backup_type;
    uint16_t s_desc_size;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_mkfs_time;
    uint32_t s_jnl_blocks[17];

    //EXT4_FEATURE_COMPAT_64BIT
    uint32_t s_blocks_count_hi;
    uint32_t s_r_blocks_count_hi;
    uint32_t s_free_blocks_count_hi;
    uint16_t s_min_extra_isize;
    uint16_t s_want_extra_isize;
    uint32_t s_flags;
    uint16_t s_raid_stride;
    uint16_t s_mmp_interval;
    uint64_t s_mmp_block;
    uint32_t s_raid_stripe_width;
    uint8_t s_log_groups_per_flex;
    uint8_t s_checksum_type;
    uint16_t s_reserved_pad;
    uint64_t s_kbytes_written;
    uint32_t s_snapshot_inum;
    uint32_t s_snapshot_id;
    uint64_t s_snapshot_r_blocks_count;
    uint32_t s_snapshot_list;
    uint32_t s_error_count;
    uint32_t s_first_error_time;
    uint32_t s_first_error_ino;
    uint64_t s_first_error_block;
    uint8_t s_first_error_func[32];
    uint32_t s_first_error_line;
    uint32_t s_last_error_time;
    uint32_t s_last_error_ino;
    uint32_t s_last_error_line;
    uint64_t s_last_error_block;
    uint8_t s_last_error_func[32];
    uint8_t s_mount_opts[64];
    uint32_t s_usr_quota_inum;
    uint32_t s_grp_quota_inum;
    uint32_t s_overhead_blocks;
    uint32_t s_backup_bgs[2];
    uint8_t s_encrypt_algos[4];
    uint8_t s_encrypt_pw_salt[16];
    uint32_t s_lpf_ino;
    uint32_t s_prj_quota_inum;
    uint32_t s_checksum_seed;
    uint8_t s_wtime_hi;
    uint8_t s_mtime_hi;
    uint8_t s_mkfs_time_hi;
    uint8_t s_lastcheck_hi;
    uint8_t s_first_error_time_hi;
    uint8_t s_last_error_time_hi;
    uint8_t s_pad[2];
    uint16_t s_encoding;
    uint16_t s_encoding_flags;
    uint16_t s_orphan_file_inum;
    uint32_t s_reserved[94];
    uint32_t s_checksum;
  };

  static_assert(sizeof(SuperBlock) == 0x400, "SuperBlock size incorrect");

  struct GroupDescriptor {
    enum FLAGS {
      EXT4_BG_INODE_UNINIT = 1,
      EXT4_BG_BLOCK_UNINIT = 2,
      EXT4_BG_INODE_ZEROED = 4
    };
    uint32_t bg_block_bitmap_lo;
    uint32_t bg_inode_bitmap_lo;
    uint32_t bg_inode_table_lo;
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lo;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;

    uint32_t bg_block_bitmap_hi;
    uint32_t bg_inode_bitmap_hi;
    uint32_t bg_inode_table_hi;
    uint16_t bg_free_blocks_count_hi;
    uint16_t bg_free_inodes_count_hi;
    uint16_t bg_used_dirs_count_hi;
    uint16_t bg_itable_unused_hi;
    uint32_t bg_exclude_bitmap_hi;
    uint16_t bg_block_bitmap_csum_hi;
    uint16_t bg_inode_bitmap_csum_hi;
    uint32_t bg_reserved;
  };

  static_assert(sizeof(GroupDescriptor) == 0x40, "GroupDescriptor size incorrect");



  SuperBlock superblock_;
  uint64_t block_size_;
  uint64_t gdt_offset_;
  uint64_t num_block_groups_;
  std::vector<GroupDescriptor> group_descriptors_;

  Ext4Partition(Disk *disk, uint64_t start_lba);
  int parseSuperblock() override;
  int read_bytes(char *buffer, uint64_t part_offset, uint64_t length);
  int read_inode(uint32_t ino, Inode &inode);
  uint64_t map_block_in_extent(const ExtExtentHeader *hdr, uint32_t lblock);
  uint64_t map_inode_block(const Inode &inode, uint32_t lblock);
  int read_inode_data(const Inode &inode, uint64_t offset, uint64_t length, char *buffer);
  int find_in_directory(const Inode &dir_inode, const std::string &name, uint32_t &out_ino);
  int resolve_path(const std::string &path, uint32_t &out_ino);
  int list_directory(uint32_t ino);
};

struct Disk {
  int fd;
  size_t block_size;
  std::vector<Partition *> partitions_;


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
    PartitionTableEntry parition_table_[4];
    uint16_t signature_;
  } __attribute__((packed)) mbr_;

  static_assert(sizeof(MBR) == 0x200, "MBR size incorrect");

  int read(char *buffer, uint64_t lba, uint64_t size);
  int parseMBR();
};

int Disk::read(char *buffer, uint64_t lba, uint64_t size) {
  assert(size%block_size == 0);
  pread(fd, buffer, size, lba*block_size);
  return 0;
}

int Disk::parseMBR() {
  read((char *)&mbr_, 0, block_size);

  for (auto x : mbr_.parition_table_) {
    printf("slba: %x\n", x.partition_start_lba_);
    printf("pt: %x\n", x.partition_type_);
    if (x.partition_type_ == 0)
      continue;
    uint64_t partition_start = x.partition_start_lba_*block_size;


    auto part = new Ext4Partition(this, x.partition_start_lba_);
    if (part->parseSuperblock() != 0) {
      delete part;
      continue;
    }
    partitions_.push_back(part);

  }

  printf("sg: %x\n", mbr_.signature_);

  return 0;
}

Ext4Partition::Ext4Partition(Disk *disk, uint64_t start_lba) : Partition(disk, start_lba) {

}
int Ext4Partition::parseSuperblock() {
  if (disk_->read((char*)&superblock_, start_lba_+1024/disk_->block_size, 1024)) return -1;
  if (superblock_.s_magic != 0xEF53) {
    printf("invalid ext4 magic\n");
    return -1;
  }
  if (superblock_.s_rev_level != 1) {
    printf("wrong ext4 revision\n");
    return -1;
  }
  if (superblock_.s_block_group_nr != 0) {
    printf("block groups not supported\n");
    return -1;
  }
  if (superblock_.s_checksum != ~crc32c((uint8_t *)&superblock_, sizeof(superblock_)-4)) {
    printf("incorrect superblock checksum\n");
    return -1;
  }
  memcpy(uuid_, &superblock_.s_uuid, 16);
  printf("volume label: %16s\n", superblock_.s_volume_name);
  printf("inode count: %x\n", superblock_.s_inodes_count);
  printf("block count: %x\n", superblock_.s_blocks_count_lo);
  printf("block group count: %x\n", superblock_.s_blocks_per_group);
  printf("block size: %llx\n", 1ULL<<(10+superblock_.s_log_block_size));
  block_size_ = 1ULL<<(10+superblock_.s_log_block_size);
  gdt_offset_ = block_size_ == 1024 ? block_size_*2 : block_size_;
  num_block_groups_ = superblock_.s_blocks_count_lo/superblock_.s_blocks_per_group;
  printf("%lu\n", num_block_groups_);
  uint8_t block[disk_->block_size];
  size_t gd_per_block = disk_->block_size/sizeof(GroupDescriptor);
  group_descriptors_.clear();
  for (size_t i = 0; num_block_groups_ > i; ++i) {
    if ((i%gd_per_block) == 0)
      disk_->read((char *)block, start_lba_+(gdt_offset_+(i*sizeof(GroupDescriptor)))/disk_->block_size, disk_->block_size);
    GroupDescriptor &gd = ((GroupDescriptor *)block)[i%gd_per_block];

    size_t checksum = gd.bg_checksum;
    gd.bg_checksum = 0;
    CRC32 crc;
    crc.push(superblock_.s_uuid, 16);
    crc.push((uint32_t)i);
    crc.push((uint8_t *)&gd, sizeof(gd));

    printf("%x\n", (uint64_t)gd.bg_free_blocks_count_hi | gd.bg_free_blocks_count_lo);
    printf("%x\n", (uint64_t)gd.bg_block_bitmap_hi | gd.bg_block_bitmap_lo);
    assert(checksum == (crc.value_&0xffffULL));

    gd.bg_checksum = checksum;
    group_descriptors_.push_back(gd);
  }

  return 0;
}

int Ext4Partition::read_bytes(char *buffer, uint64_t part_offset, uint64_t length) {
  uint64_t global_offset = start_lba_ * disk_->block_size + part_offset;
  uint64_t start_lba = global_offset / disk_->block_size;
  uint64_t byte_offset_in_block = global_offset % disk_->block_size;
  
  uint64_t end_offset = global_offset + length;
  uint64_t end_block = (end_offset + disk_->block_size - 1) / disk_->block_size;
  uint64_t num_blocks = end_block - start_lba;
  
  std::vector<char> temp(num_blocks * disk_->block_size);
  if (disk_->read(temp.data(), start_lba, num_blocks * disk_->block_size) != 0) {
    return -1;
  }
  memcpy(buffer, temp.data() + byte_offset_in_block, length);
  return 0;
}

int Ext4Partition::read_inode(uint32_t ino, Inode &inode) {
  uint32_t bg = (ino - 1) / superblock_.s_inodes_per_group;
  uint32_t index = (ino - 1) % superblock_.s_inodes_per_group;
  if (bg >= group_descriptors_.size()) return -1;
  
  const auto &gd = group_descriptors_[bg];
  uint64_t inode_table_block = ((uint64_t)gd.bg_inode_table_hi << 32) | gd.bg_inode_table_lo;
  uint64_t offset = index * superblock_.s_inode_size;
  uint64_t part_offset = inode_table_block * block_size_ + offset;
  
  size_t to_read = std::min((size_t)superblock_.s_inode_size, sizeof(Inode));
  memset(&inode, 0, sizeof(Inode));
  return read_bytes((char *)&inode, part_offset, to_read);
}

uint64_t Ext4Partition::map_block_in_extent(const ExtExtentHeader *hdr, uint32_t lblock) {
  if (hdr->eh_magic != 0xF30A) {
    printf("Invalid extent magic: %x\n", hdr->eh_magic);
    return 0;
  }
  
  uint16_t depth = hdr->eh_depth;
  if (depth == 0) {
    const ExtExtent *ext = (const ExtExtent *)(hdr + 1);
    for (uint16_t i = 0; i < hdr->eh_entries; ++i) {
      uint32_t start_lblock = ext[i].ee_block;
      uint32_t len = ext[i].ee_len;
      if (len > 32768) len -= 32768; // Uninitialized extent
      
      if (lblock >= start_lblock && lblock < start_lblock + len) {
        uint64_t start_phys = ((uint64_t)ext[i].ee_start_hi << 32) | ext[i].ee_start_lo;
        return start_phys + (lblock - start_lblock);
      }
    }
    return 0;
  } else {
    const ExtExtentIdx *idx = (const ExtExtentIdx *)(hdr + 1);
    int target_idx = -1;
    for (uint16_t i = 0; i < hdr->eh_entries; ++i) {
      if (idx[i].ei_block <= lblock) {
        target_idx = i;
      } else {
        break;
      }
    }
    if (target_idx == -1) return 0;
    
    uint64_t next_level_block = ((uint64_t)idx[target_idx].ei_leaf_hi << 32) | idx[target_idx].ei_leaf_lo;
    std::vector<char> block_buf(block_size_);
    if (read_bytes(block_buf.data(), next_level_block * block_size_, block_size_) != 0) {
      return 0;
    }
    
    return map_block_in_extent((const ExtExtentHeader *)block_buf.data(), lblock);
  }
}

uint64_t Ext4Partition::map_inode_block(const Inode &inode, uint32_t lblock) {
  if (inode.i_flags & 0x80000) {
    const ExtExtentHeader *hdr = (const ExtExtentHeader *)inode.i_block;
    return map_block_in_extent(hdr, lblock);
  } else {
    printf("Non-extent inode mapping not fully supported (flags: %x)\n", inode.i_flags);
    if (lblock < 12) {
      return inode.i_block[lblock];
    }
    return 0;
  }
}

int Ext4Partition::read_inode_data(const Inode &inode, uint64_t offset, uint64_t length, char *buffer) {
  uint64_t file_size = ((uint64_t)inode.i_size_high << 32) | inode.i_size_lo;
  if (offset >= file_size) return 0;
  if (offset + length > file_size) {
    length = file_size - offset;
  }
  
  uint64_t bytes_read = 0;
  while (bytes_read < length) {
    uint64_t cur_offset = offset + bytes_read;
    uint32_t lblock = cur_offset / block_size_;
    uint32_t offset_in_block = cur_offset % block_size_;
    uint32_t to_read = std::min((uint64_t)block_size_ - offset_in_block, length - bytes_read);
    
    uint64_t pblock = map_inode_block(inode, lblock);
    if (pblock == 0) {
      memset(buffer + bytes_read, 0, to_read);
    } else {
      std::vector<char> block_buf(block_size_);
      if (read_bytes(block_buf.data(), pblock * block_size_, block_size_) != 0) {
        return -1;
      }
      memcpy(buffer + bytes_read, block_buf.data() + offset_in_block, to_read);
    }
    bytes_read += to_read;
  }
  return bytes_read;
}

int Ext4Partition::find_in_directory(const Inode &dir_inode, const std::string &name, uint32_t &out_ino) {
  if ((dir_inode.i_mode & 0xF000) != 0x4000) {
    return -1;
  }
  
  uint64_t file_size = ((uint64_t)dir_inode.i_size_high << 32) | dir_inode.i_size_lo;
  std::vector<char> block_buf(block_size_);
  
  uint64_t num_blocks = (file_size + block_size_ - 1) / block_size_;
  for (uint32_t lblock = 0; lblock < num_blocks; ++lblock) {
    uint64_t pblock = map_inode_block(dir_inode, lblock);
    if (pblock == 0) continue;
    
    if (read_bytes(block_buf.data(), pblock * block_size_, block_size_) != 0) {
      return -1;
    }
    
    uint32_t offset = 0;
    while (offset < block_size_) {
      const ExtDirEntry *entry = (const ExtDirEntry *)(block_buf.data() + offset);
      if (entry->rec_len < 8 || offset + entry->rec_len > block_size_) {
        break;
      }
      
      if (entry->inode != 0 && entry->name_len > 0) {
        std::string entry_name(entry->name, entry->name_len);
        if (entry_name == name) {
          out_ino = entry->inode;
          return 0;
        }
      }
      
      offset += entry->rec_len;
    }
  }
  return -1;
}

int Ext4Partition::resolve_path(const std::string &path, uint32_t &out_ino) {
  uint32_t cur_ino = 2;
  
  std::vector<std::string> parts;
  std::string current;
  for (char c : path) {
    if (c == '/') {
      if (!current.empty()) {
        parts.push_back(current);
        current.clear();
      }
    } else {
      current.push_back(c);
    }
  }
  if (!current.empty()) {
    parts.push_back(current);
  }
  
  for (const auto &part : parts) {
    Inode dir_inode;
    if (read_inode(cur_ino, dir_inode) != 0) {
      return -1;
    }
    uint32_t next_ino;
    if (find_in_directory(dir_inode, part, next_ino) != 0) {
      return -1;
    }
    cur_ino = next_ino;
  }
  
  out_ino = cur_ino;
  return 0;
}

int Ext4Partition::list_directory(uint32_t ino) {
  Inode dir_inode;
  if (read_inode(ino, dir_inode) != 0) {
    return -1;
  }
  if ((dir_inode.i_mode & 0xF000) != 0x4000) {
    printf("Not a directory!\n");
    return -1;
  }
  
  uint64_t file_size = ((uint64_t)dir_inode.i_size_high << 32) | dir_inode.i_size_lo;
  std::vector<char> block_buf(block_size_);
  
  uint64_t num_blocks = (file_size + block_size_ - 1) / block_size_;
  for (uint32_t lblock = 0; lblock < num_blocks; ++lblock) {
    uint64_t pblock = map_inode_block(dir_inode, lblock);
    if (pblock == 0) continue;
    
    if (read_bytes(block_buf.data(), pblock * block_size_, block_size_) != 0) {
      return -1;
    }
    
    uint32_t offset = 0;
    while (offset < block_size_) {
      const ExtDirEntry *entry = (const ExtDirEntry *)(block_buf.data() + offset);
      if (entry->rec_len < 8 || offset + entry->rec_len > block_size_) {
        break;
      }
      
      if (entry->inode != 0 && entry->name_len > 0) {
        std::string entry_name(entry->name, entry->name_len);
        printf("  Ino %u: %s (type %d)\n", entry->inode, entry_name.c_str(), entry->file_type);
      }
      
      offset += entry->rec_len;
    }
  }
  return 0;
}

int main() {
  Disk disk;
  disk.fd = open("disk.img", O_RDWR);
  disk.block_size=512;
  assert(disk.fd != -1);
  disk.parseMBR();

  for (auto p : disk.partitions_) {
    auto part = static_cast<Ext4Partition *>(p);
    
    printf("\n--- Reading root directory (/) ---\n");
    part->list_directory(2);
    
    printf("\n--- Resolving /CMakeLists.txt ---\n");
    uint32_t cmake_ino;
    if (part->resolve_path("/CMakeLists.txt", cmake_ino) == 0) {
      printf("Found /CMakeLists.txt at inode %u\n", cmake_ino);
      Inode cmake_inode;
      if (part->read_inode(cmake_ino, cmake_inode) == 0) {
        uint64_t size = ((uint64_t)cmake_inode.i_size_high << 32) | cmake_inode.i_size_lo;
        printf("File size: %lu bytes\n", size);
        std::vector<char> file_data(size + 1, 0);
        part->read_inode_data(cmake_inode, 0, size, file_data.data());
        printf("--- Content of /CMakeLists.txt ---\n%s\n--------------------------------------\n", file_data.data());
      }
    } else {
      printf("Failed to resolve /CMakeLists.txt\n");
    }

    printf("\n--- Reading directory /driver ---\n");
    uint32_t driver_ino;
    if (part->resolve_path("/driver", driver_ino) == 0) {
      part->list_directory(driver_ino);
    }

    printf("\n--- Resolving /driver/ahci.cpp ---\n");
    uint32_t ahci_ino;
    if (part->resolve_path("/driver/ahci.cpp", ahci_ino) == 0) {
      printf("Found /driver/ahci.cpp at inode %u\n", ahci_ino);
      Inode ahci_inode;
      if (part->read_inode(ahci_ino, ahci_inode) == 0) {
        uint64_t size = ((uint64_t)ahci_inode.i_size_high << 32) | ahci_inode.i_size_lo;
        printf("File size: %lu bytes\n", size);
        std::vector<char> file_data(size + 1, 0);
        part->read_inode_data(ahci_inode, 0, size, file_data.data());
        printf("--- Content of /driver/ahci.cpp (first 300 bytes) ---\n%.300s...\n--------------------------------------\n", file_data.data());
      }
    } else {
      printf("Failed to resolve /driver/ahci.cpp\n");
    }
  }
}
