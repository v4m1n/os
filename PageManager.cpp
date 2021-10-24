#include "PageManager.h"
#include "debug.h"

namespace pm {
PageManager instance;

PageManager::PageManager() {
  static size_t call = 0;
  dbg::panic_assert(call++ == 0, "trying to construct a second PageManager\n");
}

}

