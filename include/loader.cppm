module;

#include <elf.h>

export module loader;

import vfs;
import sync;
import array;

struct Loader {
  vfs::VfsNode *file_ = nullptr;
  mutex file_lock_;

  Loader(const Loader &) = delete;
  Loader& operator=(const Loader&) = delete;

  uint64_t entry_;
  
  array<Elf64_Phdr> phdrs_;
  

  bool init(const char *path);
  ~Loader();
  
};

