#pragma once

#include <tui++/Rectangle.h>

#include <mutex>
#include <memory>
#include <unordered_map>

namespace tui {

class Component;

class RepaintManager: public std::enable_shared_from_this<RepaintManager> {
  std::unordered_map<std::shared_ptr<Component>, Rectangle> dirty_regions;
  std::mutex mutex;
public:
  static inline std::shared_ptr<RepaintManager> single = std::make_shared<RepaintManager>();

public:
  RepaintManager();

public:
  void add_dirty_region(std::shared_ptr<Component> const &c, Rectangle const &bounds);

  void add_dirty_region(std::shared_ptr<Component> const &c, int x, int y, int width, int height) {
    add_dirty_region(c, { x, y, width, height });
  }

private:
  bool extend_dirty_region(std::shared_ptr<Component> const &c, Rectangle const &bounds);
  void repaint_dirty_regions();
};

}
