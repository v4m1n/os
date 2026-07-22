module;

#include <elf.h>
#include <cstddef>

export module loader;

import vfs;
import sync;
import array;
import utility;
import registers;

export struct Loader {
  vfs::VfsNode *file_ = nullptr;
  mutex region_lock_;
  size_t entry_;

  Loader() = default;
  Loader(const Loader &) = delete;
  Loader& operator=(const Loader&) = delete;


  struct VirtMemRegion {
    uint64_t start_;
    uint64_t size_;
    uint64_t file_offset_;
    uint64_t file_size_;
    util::shared_ptr<vfs::VfsNode> file_;
    bool read_;
    bool write_;
    bool exec_;
    util::shared_ptr<VirtMemRegion> next_;
  };
  util::shared_ptr<VirtMemRegion> mappings_;

  bool insertRegion(util::shared_ptr<VirtMemRegion> n);
  util::shared_ptr<VirtMemRegion> findRegion(uint64_t addr);

  
  bool loadMemory(size_t addr);

  void dumpRegions();
  

  bool init(const char *path);
  void handlePagefault(size_t addr, bool present, bool write, bool execute, bool user, thrd::registers *regs);
  ~Loader();
  
};

