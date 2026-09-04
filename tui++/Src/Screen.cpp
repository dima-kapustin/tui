#include <tui++/Screen.h>
#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <tui++/util/log.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace tui {

namespace {

// The largest number of separate damaged regions a repaint pass will flush.
// Beyond this the regions are merged (with the cheapest neighbour) so a
// pathological burst cannot build an unbounded list.
constexpr size_t MAX_DAMAGE_REGIONS = 8;

// Regions are merged when the union's area stays within this factor of the
// sum of the two areas. Overlapping/adjacent regions have ratio ~= 1 and are
// merged; two small regions at opposite corners of the screen have a large
// ratio and stay separate, so one pass does not flush one huge bounding box
// full of unchanged content.
constexpr long long MERGE_ALLOWANCE = 2;

long long area(Rectangle const &r) {
  return 1LL * r.width * r.height;
}

Rectangle bbox(Rectangle const &a, Rectangle const &b) {
  return a | b;
}

} // namespace

void Screen::paint(Graphics &g) {
  std::unique_lock lock(this->windows_mutex);
  for (auto &&window : this->windows) {
    window->paint(g);
  }
}

std::shared_ptr<Window> Screen::get_window_at(int x, int y) const {
  std::unique_lock lock(this->windows_mutex);
  // Windows are painted in list order, so the last one is on top; hit-test
  // from the top down so a popup receives clicks over the window behind it.
  // contains() takes window-local coordinates, so offset by the window's
  // screen position.
  for (auto i = this->windows.rbegin(); i != this->windows.rend(); ++i) {
    if ((*i)->contains(x - (*i)->get_x(), y - (*i)->get_y())) {
      return *i;
    }
  }
  return { };
}

void Screen::show_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (std::find(this->windows.begin(), this->windows.end(), window) == this->windows.end()) {
    this->windows.emplace_back(window);
    if (this->windows.size() == 1) {
      focus(window, nullptr);
    }

    refresh();
  }
}

void Screen::hide_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (auto pos = std::find(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
    this->windows.erase(pos, this->windows.end());
    // Repaint what the hidden window uncovered (e.g. a closed popup menu);
    // pixel-level screens flush the repaint, text screens are unaffected.
    refresh();
  } else {
    throw std::runtime_error("window not visible");
  }
}

void Screen::to_front(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (not this->windows.empty()) {
    if (this->windows.back() != window) {
      auto front = this->windows.front();
      if (auto pos = std::find(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
        this->windows.erase(pos, this->windows.end());
        this->windows.emplace_back(window);
        focus(window, front);
      } else {
        throw std::runtime_error("window not visible");
      }
    }
  } else {
    this->windows.emplace_back(window);
    focus(window, nullptr);
    refresh();
  }
}

void Screen::focus(const std::shared_ptr<Window> &gained, const std::shared_ptr<Window> &lost) {
  post_system<WindowEvent>(lost, WindowEvent::WINDOW_LOST_FOCUS, gained);
  post_system<WindowEvent>(gained, WindowEvent::WINDOW_GAINED_FOCUS, lost);
}

void Screen::dispatch_event(Event &event) {
  // The duration is always measured (a steady_clock read is negligible); the
  // log macro below no-ops when logging is disabled.
  auto t0 = std::chrono::steady_clock::now();

  if (event.id == InvocationEvent::INVOCATION) {
    static_cast<InvocationEvent&>(event).dispatch();
  } else if (auto c = std::dynamic_pointer_cast<Component>(event.source)) {
    c->dispatch_event(event);
  }

  // Component::dispatch_event already logs the "event: ..." line via
  // log_event_ln; here the cost of handling it is logged, so an enabled event
  // log reads as an ordered, timestamped history of events with their cost.
  auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  log_dispatch_ln("id " << event.id.id << " took " << ms << " ms");
}

void Screen::add_damage(Rectangle const &rect) {
  if (rect.empty()) {
    return;
  }

  auto &regions = this->damaged_regions;

  // Pick the existing region whose union with the new rectangle grows least:
  // merging overlapping or neighbouring damage is what costs the least.
  auto best = regions.size();
  auto best_area = std::numeric_limits<long long>::max();
  auto rect_area = area(rect);
  for (auto i = size_t { 0 }; i < regions.size(); ++i) {
    auto merged_area = area(bbox(rect, regions[i]));
    if (merged_area < best_area) {
      best_area = merged_area;
      best = i;
    }
  }

  auto can_merge = false;
  if (best < regions.size()) {
    // Merging is cheap when the union covers little more than the two regions
    // themselves; if the new rectangle is far away the union is a huge box of
    // unchanged content and it is cheaper to keep the regions separate.
    can_merge = best_area <= (rect_area + area(regions[best])) * MERGE_ALLOWANCE;
  }

  if (can_merge or regions.size() >= MAX_DAMAGE_REGIONS) {
    if (best < regions.size()) {
      regions[best] = bbox(rect, regions[best]);
    } else {
      regions.emplace_back(rect);
    }
  } else {
    regions.emplace_back(rect);
  }

  // Overlapping regions would be flushed twice; coalesce them. The list is
  // capped at MAX_DAMAGE_REGIONS, so this stays O(n^2) on a tiny list.
  auto changed = true;
  while (changed and regions.size() > 1) {
    changed = false;
    for (auto i = size_t { 0 }; i < regions.size() and not changed; ++i) {
      for (auto j = i + 1; j < regions.size(); ++j) {
        if (not (regions[i] & regions[j]).empty()) {
          regions[i] = bbox(regions[i], regions[j]);
          regions.erase(regions.begin() + std::ptrdiff_t(j));
          changed = true;
          break;
        }
      }
    }
  }

  // Schedule the repaint as an event on the same queue that carries the mouse
  // and key events, with at most one such invocation pending. The paint then
  // runs on the dispatch thread, ordered with the events around it, and every
  // repaint request that arrives before it runs extends the damage it paints.
  if (not this->repaint_event_pending) {
    this->repaint_event_pending = true;
    post([this] {
      this->repaint_damaged();
    });
  }
}

void Screen::repaint_damaged() {
  this->repaint_event_pending = false;
  if (this->damaged_regions.empty()) {
    return;
  }

  auto regions = std::exchange(this->damaged_regions, { });
  log_repaint_ln(regions.size() << " region(s)");

  for (auto const &region : regions) {
    if (region.empty()) {
      continue;
    }
    auto t0 = std::chrono::steady_clock::now();
    repaint_region(region);
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    log_repaint_ln("  region (" << region.x << ", " << region.y << " " << region.width << "x" << region.height << ") took " << ms << " ms");
  }
}

void Screen::add_listener(const EventTypeMask &event_mask, const std::shared_ptr<EventListener<Event>> &listener) {
  for (auto i = selective_listeners.begin(); i != selective_listeners.end(); ++i) {
    if (i->listener == listener) {
      i->event_mask |= event_mask;
      return;
    }
  }
  selective_listeners.emplace_back(event_mask, listener);
}

void Screen::remove_listener(const std::shared_ptr<EventListener<Event>> &listener) {
  for (auto i = selective_listeners.begin(); i != selective_listeners.end(); ++i) {
    if (i->listener == listener) {
      selective_listeners.erase(i);
      break;
    }
  }
}

void Screen::notify_listeners(Event &e) {
  for (auto&& [event_mask, listener] : selective_listeners) {
    if (event_mask & e.id) {
      listener->event_dispatched(e);
    }
  }
}

}
