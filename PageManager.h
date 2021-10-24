#pragma once

namespace pm {

class PageManager {
  public:
    PageManager();

    PageManager(PageManager &) = delete;
    PageManager(PageManager &&) = delete;
    PageManager &operator=(PageManager &) = delete;
    PageManager &operator=(PageManager &&) = delete;
};

extern PageManager instance;

}
