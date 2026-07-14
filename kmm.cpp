module;
#include <cstdint>
#include <cstddef>

module kmm;
import knew;
import string;
import sync;
import pmm;
import debug;
import vmm;

namespace kmm {
  struct MemCache;
  struct Slab;
  struct Object {
    struct Slab *slab_;
    uint8_t free_ : 1;
    alignas(16) void *memory_[];
  };
  struct Slab {
    Slab() = default;
    mutex lock_;
    MemCache *cache_;
    Slab *next_;
    Slab *prev_;
    size_t num_alloc_;
    Object *free_;
    Object memory_[];
  };
  struct MemCache {
    mutex lock_;
    size_t size_;
    size_t slab_size_;
    Slab *full_;
    Slab *partial_;
    Slab *free_;
  };

  void moveListElement(Slab *&dest, Slab *&src) {
    dbg::panic_assert(src, "src can not be nullptr\n");
    dbg::panic_assert(dest != src, "src and dest are the same\n");
    Slab *old_dest = dest;
    Slab *old_src = src;
    Slab *new_src = src->next_;
    if (old_src->prev_) {
      old_src->prev_->next_ = old_src->next_;
    }
    if (old_src->next_) {
      old_src->next_->prev_ = old_src->prev_;
    }
    old_src->next_ = old_dest;
    old_src->prev_ = nullptr;
    if (old_dest) {
      old_src->prev_ = old_dest->prev_;
      if (old_dest->prev_) {
        old_dest->prev_->next_ = old_src;
      }
      old_dest->prev_ = old_src;
    }
    dest = old_src;
    src = new_src;
    dbg::panic_assert(src != old_src, "old and new src are the same\n");
  }

  Slab *allocateSlab(const size_t slab_size, const size_t size) {
    dbg::panic_assert(slab_size%PAGE_SIZE == 0, "slab size not a multiple of page size\n");
    size_t pages = pmm::allocPFN(slab_size);
    auto slab = vmm::pageAddress<Slab *>(pages);
    memset(slab, 0, sizeof(Slab));
    new (slab) Slab;
    slab->free_ = slab->memory_;
    const auto end = reinterpret_cast<size_t>(slab) + slab_size - (sizeof(Object) + size);
    auto current = reinterpret_cast<size_t>(slab->memory_);

    dbg::panic_assert(current <= end, "slab is too small for even a single entry\n");
    Object **next_ptr = &slab->free_;

    while (current <= end) {
      auto ptr = reinterpret_cast<Object *>(current);
      *next_ptr = ptr;
      ptr->free_ = 1;
      ptr->slab_ = slab;
      next_ptr = reinterpret_cast<Object **>(&ptr->memory_[0]);
      current += (sizeof(Object) + size);
    }
    *next_ptr = nullptr;

    return slab;
  }
  void *allocateSlabChunk(Slab * const slab) {
    slab->lock_.lock();
    auto next = slab->free_;
    dbg::panic_assert(next, "trying to allocate on a full slab {}\n", slab);
    dbg::panic_assert(next->free_, "block on free list not free\n");
    next->free_ = 0;
    ++slab->num_alloc_;
    slab->free_ = static_cast<Object *>(next->memory_[0]);
    slab->lock_.unlock();
    return next->memory_;
  }
  void freeSlabChunk(Object * const obj) {
    auto slab = obj->slab_;
    dbg::panic_assert(!obj->free_, "double free\n");
    dbg::panic_assert(slab->num_alloc_, "free called even though there are no free objects\n");
    obj->free_ = 1;
    --slab->num_alloc_;
    obj->memory_[0] = slab->free_;
    slab->free_ = obj;
  }

  void kfree_slab(void * const ptr) {
    auto obj = reinterpret_cast<Object *>(reinterpret_cast<size_t>(ptr)-sizeof(Object));
    auto slab = obj->slab_;
    auto cache = slab->cache_;
    cache->lock_.lock();
    slab->lock_.lock();
    if (slab->num_alloc_ == 1) {
      auto &tmp = slab == cache->partial_ ? cache->partial_ : (slab == cache->full_ ? cache->full_ : slab);
      moveListElement(cache->free_, tmp);
    }
    else if (slab->free_ == nullptr) {
      auto &tmp = slab->prev_ ? slab : cache->full_;
      moveListElement(cache->partial_, tmp);
    }
    cache->lock_.unlock();
    freeSlabChunk(obj);
    dbg::panic_assert(obj->slab_->free_!=nullptr, "slab is full even though an element was freed\n");
    slab->lock_.unlock();
  }
  
  void *kmalloc_slab(MemCache &cache) {
    cache.lock_.lock();
    if (cache.partial_ == nullptr) {
      if (cache.free_) {
        moveListElement(cache.partial_, cache.free_);
      }
      else {
        auto slab = allocateSlab(cache.slab_size_, cache.size_);
        slab->cache_ = &cache;
        moveListElement(cache.partial_, slab);
      }
    }
    dbg::panic_assert(cache.partial_, "partial slab list empty\n");
    
    void *memory = allocateSlabChunk(cache.partial_);
    if (cache.partial_->free_ == nullptr) {
      moveListElement(cache.full_, cache.partial_);
    } 
    cache.lock_.unlock();
    return memory;
  }

  void testslab() {
    for (size_t i = 0; i < 4096; ++i) {
      void *mem[128];
      for (size_t j = 0; j < 128; ++j) {
        mem[j] = kmalloc(i);
        dbg::printf("mem: {}\n", mem[j]);
      }
      for (size_t j = 127; j > 30; --j) {
        kfree(mem[j]);
      }
    }
  }

  static constinit MemCache cache[] = {
    {{}, 32,     PAGE_SIZE,     nullptr, nullptr, nullptr},
    {{}, 64,     PAGE_SIZE,     nullptr, nullptr, nullptr},
    {{}, 128,    PAGE_SIZE*2,   nullptr, nullptr, nullptr},
    {{}, 256,    PAGE_SIZE*2,   nullptr, nullptr, nullptr},
    {{}, 512,    PAGE_SIZE*4,   nullptr, nullptr, nullptr},
    {{}, 1024,   PAGE_SIZE*4,   nullptr, nullptr, nullptr},
    {{}, 2048,   PAGE_SIZE*16,  nullptr, nullptr, nullptr},
    {{}, 4096,   PAGE_SIZE*16,  nullptr, nullptr, nullptr},
    {{}, 8192,   PAGE_SIZE*32,  nullptr, nullptr, nullptr},
    {{}, 16384,  PAGE_SIZE*64,  nullptr, nullptr, nullptr},
    {{}, 32768,  PAGE_SIZE*128, nullptr, nullptr, nullptr},
    {{}, 65536,  PAGE_SIZE*128, nullptr, nullptr, nullptr},
    {{}, 131072, PAGE_SIZE*128, nullptr, nullptr, nullptr},
  };
  void kmalloc_init() {
    static size_t call = 0;
    dbg::panic_assert(call++ == 0, "trying to init kmalloc twice\n");
  }
  void *kmalloc(size_t size) {
    MemCache *found = nullptr;
    for (auto &x : cache) {
      if (x.size_ > size) {
        found = &x;
        break;
      }
    }
    dbg::panic_assert(found, "no fitting cache found for size {}\n", size);
    return kmalloc_slab(*found);
  }

  void kfree(void *ptr) {
    if (!ptr) return;
    kfree_slab(ptr);
  }
}
