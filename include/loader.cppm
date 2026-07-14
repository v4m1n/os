module;

#include <elf.h>
#include <cstddef>

export module loader;

import vfs;
import sync;
import array;

export struct Loader {
  vfs::VfsNode *file_ = nullptr;
  mutex file_lock_;

  Loader() = default;
  Loader(const Loader &) = delete;
  Loader& operator=(const Loader&) = delete;

  uint64_t entry_;
  
  array<Elf64_Phdr> phdrs_;
  

  bool init(const char *path);
  void handlePagefault(size_t addr, bool present, bool write, bool execute);
  ~Loader();
  
};

