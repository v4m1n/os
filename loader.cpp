module;

#include <algorithm>
#include <cstdint>

module loader;

import debug;
import vfs;
import string;
import array;
import scheduler;

#include <elf.h>

bool Loader::init(const char *path) {
  dbg::panic_assert(path, "loader path is null\n");

  file_ = vfs::open(path);
  if (!file_) goto fail;

  elf64_hdr hdr;

  if (file_->read(0, sizeof(elf64_hdr), &hdr) != (int64_t)sizeof(elf64_hdr))
    goto fail;

  if (memcmp(hdr.e_ident, ELFMAG, SELFMAG) != 0 
      || hdr.e_ident[4] != ELFCLASS64
      || hdr.e_type != ET_EXEC
      || hdr.e_phentsize != sizeof(Elf64_Phdr))
    goto fail;

  uint64_t phoff;
  uint64_t ph_size;
  phoff = hdr.e_phoff;
  ph_size = hdr.e_phentsize * hdr.e_phnum;
  if (file_->getSize() < phoff
      || phoff + ph_size < phoff
      || file_->getSize() < phoff + ph_size)
    goto fail;
  
  phdrs_.resize(hdr.e_phnum);


  if (file_->read(hdr.e_phoff, ph_size, phdrs_.data()) != (int64_t)ph_size)
    goto fail;

  entry_ = hdr.e_entry;


  return true;
fail:
  delete file_;
  return false;
}
void Loader::handlePagefault(size_t addr, bool present, bool write, bool execute) {
  auto thrd = sched::getCurrentThread();
  dbg::printf("pagefualt @ {} by {}, p {}, w {}, e {}\n", addr, thrd, present, write, execute);
}

Loader::~Loader() {
  delete file_;
}
