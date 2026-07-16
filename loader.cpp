module;

#include <algorithm>
#include <cstdint>

module loader;

import debug;
import vfs;
import string;
import array;
import scheduler;
import vmm;
import pmm;

#include <elf.h>

bool Loader::init(const char *path) {
  dbg::panic_assert(path, "loader path is null\n");

  elf64_phdr *phdrs;

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
 
  phdrs = new elf64_phdr[hdr.e_phnum];


  if (file_->read(hdr.e_phoff, ph_size, phdrs) != (int64_t)ph_size)
    goto fail;

  std::for_each(phdrs, phdrs + hdr.e_phnum, [this, path](auto &hdr) {
    if (hdr.p_type != PT_LOAD) return;
    if ((hdr.p_vaddr&(PAGE_SIZE-1)) != (hdr.p_offset&(PAGE_SIZE-1))) return;
      auto off = hdr.p_vaddr&(PAGE_SIZE-1);
      auto region = util::shared_ptr<VirtMemRegion>(new VirtMemRegion{
        .start_ = hdr.p_vaddr-off,
        .size_ = ((hdr.p_memsz-1)&~(PAGE_SIZE-1)) + PAGE_SIZE, //round up to multiple page size
        .file_offset_ = hdr.p_offset-off,
        .file_size_ = hdr.p_filesz+off,
        .file_ = util::shared_ptr(vfs::open(path)),
        .read_ = static_cast<bool>(hdr.p_flags&PF_R),
        .write_ = static_cast<bool>(hdr.p_flags&PF_W),
        .exec_ = static_cast<bool>(hdr.p_flags&PF_X),
        .next_ = nullptr

    });
    insertRegion(region);


  });

  insertRegion(util::shared_ptr(new VirtMemRegion{
    .start_ = vmm::AddressSpace::USER_END-PAGE_SIZE*1024,
    .size_ = PAGE_SIZE*1024,
    .file_offset_ = 0,
    .file_size_ = 0,
    .file_ = nullptr,
    .read_ = true,
    .write_ = true,
    .exec_ = false,
    .next_ = nullptr
  }));




  entry_ = hdr.e_entry;

  dumpRegions();


  return true;
fail:
  delete file_;
  return false;
}
bool Loader::loadMemory(size_t addr) {
  auto reg = findRegion(addr);

  if (!reg) return false;

  size_t vpn = addr/PAGE_SIZE;
  size_t pfn = pmm::allocZeroPFN();
  auto page = vmm::pageAddress<char *>(pfn);

  addr = addr/PAGE_SIZE*PAGE_SIZE;

  auto diff = addr-reg->start_;
  if (reg->file_ && diff < reg->file_size_) {
    auto rd_size = std::min(reg->file_size_-diff, PAGE_SIZE);
    reg->file_->read(reg->file_offset_+diff, rd_size, page);
  }

  auto thrd = sched::getCurrentThread();
  if(!thrd->address_space_->mapPFN(vpn, pfn, reg->read_, !reg->exec_)) {
    pmm::freePFN(pfn);
  }
  dbg::printf("page mapped {}\n", addr);

  return true;
}

void Loader::handlePagefault(size_t addr, bool present, bool write, bool execute) {
  dbg::panic_assert(!present, "not implemented");
  auto thrd = sched::getCurrentThread();
  
  dbg::printf("pagefault @ {} by {}, p {}, w {}, e {}\n", addr, thrd, present, write, execute);
  loadMemory(addr);
}

Loader::~Loader() {
  while (mappings_) {
    util::shared_ptr<VirtMemRegion> tmp = mappings_;
    mappings_ = tmp->next_;
  }
}
void Loader::dumpRegions() {
  dbg::printf("Mappings: \n");
  
  auto tmp = mappings_;
  while (tmp) {
    dbg::printf("  {}-{}, {}-{}, r {}, w {}, e{}\n", tmp->start_, tmp->start_+tmp->size_-1, tmp->file_offset_, tmp->file_offset_+tmp->file_size_-1, tmp->read_, tmp->write_, tmp->exec_);
    tmp = tmp->next_;
  }
}
bool Loader::insertRegion(util::shared_ptr<VirtMemRegion> n) {
  if ((n->start_ & (PAGE_SIZE-1)) || (n->size_ & (PAGE_SIZE-1)) || !n->size_) return false;
 
  lock_guard guard(region_lock_);

  util::shared_ptr<VirtMemRegion> *cur = &mappings_;
  for (;(*cur) && (*cur)->start_ > n->start_; cur = &(*cur)->next_);

  if (!*cur) goto done;

  if ((*cur)->start_+(*cur)->size_ > n->start_) return false;

  if (!(*cur)->next_) goto done;

  if (n->start_+n->size_ > (*cur)->next_->start_) return false;

  done:
  n->next_ = *cur;
  *cur = n;
  return true;
}
util::shared_ptr<Loader::VirtMemRegion> Loader::findRegion(uint64_t addr) {
  lock_guard guard(region_lock_);
  auto cur = mappings_;
  for(; cur; cur = cur->next_) {
    if (cur->start_ <= addr && cur->start_ + cur->size_ > addr) break;
    if (cur->start_ < addr) return nullptr;
  }
  return cur;
}
