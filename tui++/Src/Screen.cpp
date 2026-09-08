#include <tui++/Screen.h>
#include <tui++/Timer.h>
#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <tui++/util/log.h>
#include <tui++/util/typeid.h>

#include <chrono>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace tui {

namespace {

// The largest number of separate damaged regions a repaint pass will flush.
// Beyond this the regions are merged (with the cheapest neighbour) so a
// pathological burst cannot build an unbounded list.
constexpr size_t MAX_DAMAGE_REGIONS = 8;

// Regions are merged only when they overlap or touch (their union adds no
// undamaged area). Two regions that merely sit close together stay separate:
// gap-filling a damaged band across a fixed row (a horizontal scroll bar, a
// status line, a border) would feed that fixed row to the terminal-scroll
// optimization, which scrolls it with the content and then re-emits it back --
// a visible flicker. Keeping the scrolling surface as its own region lets a
// wheel scroll use the terminal scroll on the clean viewport band while the
// adjacent fixed UI is repainted in place. The MAX_DAMAGE_REGIONS cap below
// still bounds the list when a burst would exceed it.
constexpr long long MERGE_ALLOWANCE = 1;

long long area(Rectangle const &r) {
  return 1LL * r.width * r.height;
}

Rectangle bbox(Rectangle const &a, Rectangle const &b) {
  return a | b;
}

// The concrete event type as a short name ("MouseMoveEvent",
// "InvocationEvent", ...), so dispatch lines say what is being dispatched.
std::string event_type_name(Event const &event) {
  auto name = util::demangle(typeid(event).name());
  if (auto pos = name.rfind("::"); pos != std::string::npos) {
    name.erase(0, pos + 2);
  }
  return name;
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
  log_dispatch_ln(event_type_name(event) << " id " << event.id.id << " took " << ms << " ms");
}

void Screen::add_damage(Rectangle const &rect) {
  add_damage(rect, { });
}

void Screen::add_damage(Rectangle const &rect, std::shared_ptr<Component> const &source) {
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
    auto merged_area = area(bbox(rect, regions[i].rect));
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
    can_merge = best_area <= (rect_area + area(regions[best].rect)) * MERGE_ALLOWANCE;
  }

  if (can_merge or regions.size() >= MAX_DAMAGE_REGIONS) {
    if (best < regions.size()) {
      regions[best].rect = bbox(rect, regions[best].rect);
      // The region now covers this request too; the latest requester is the
      // one the flush log should blame.
      regions[best].source = source;
    } else {
      regions.emplace_back(DamagedRegion { rect, source });
    }
  } else {
    regions.emplace_back(DamagedRegion { rect, source });
  }

  // Overlapping regions would be flushed twice; coalesce them. The list is
  // capped at MAX_DAMAGE_REGIONS, so this stays O(n^2) on a tiny list.
  auto changed = true;
  while (changed and regions.size() > 1) {
    changed = false;
    for (auto i = size_t { 0 }; i < regions.size() and not changed; ++i) {
      for (auto j = i + 1; j < regions.size(); ++j) {
        if (not (regions[i].rect & regions[j].rect).empty()) {
          regions[i].rect = bbox(regions[i].rect, regions[j].rect);
          regions[i].source = source;
          regions.erase(regions.begin() + std::ptrdiff_t(j));
          changed = true;
          break;
        }
      }
    }
  }

  // Schedule the repaint: as an event on the same queue that carries the
  // mouse and key events (the default), or on the frame clock when the event
  // loop enabled it (see repaint_interval). At most one paint is pending at a
  // time, and every repaint request that arrives before it runs extends the
  // damage it paints.
  if (not this->repaint_event_pending) {
    this->repaint_event_pending = true;
    if (this->repaint_interval.count() > 0) {
      this->repaint_due = std::chrono::steady_clock::now() + this->repaint_interval;
    } else {
      post([this] {
        this->repaint_damaged();
      });
    }
  }
}

void Screen::add_timer(Timer *timer) {
  this->timers.emplace_back(Timer::Clock::now() + timer->period, timer);
}

void Screen::remove_timer(Timer *timer) {
  std::erase_if(this->timers, [timer](PendingTimer const &entry) {
    return entry.timer == timer;
  });
}

void Screen::run_pending_timers() {
  if (this->timers.empty()) {
    return;
  }

  // Fire the due entries in order. The entry is removed and the callback
  // copied before it runs: a tick may stop or destroy its own timer (and may
  // start or stop any other), and the vector must stay consistent while the
  // loop below keeps iterating it.
  auto now = Timer::Clock::now();
  for (auto i = std::size_t { 0 }; i < this->timers.size();) {
    auto &entry = this->timers[i];
    if (entry.due > now) {
      ++i;
      continue;
    }

    auto timer = entry.timer;
    auto tick = timer->tick;
    this->timers.erase(this->timers.begin() + std::ptrdiff_t(i));

    // Repeating timers re-queue themselves before the callback runs, so a
    // tick that stops the timer cancels the new entry again.
    if (timer->running) {
      this->timers.emplace_back(now + timer->period, timer);
    }

    tick();
  }
}

void Screen::repaint_damaged() {
  this->repaint_event_pending = false;
  if (this->damaged_regions.empty()) {
    return;
  }

  auto regions = std::exchange(this->damaged_regions, { });
  log_repaint_ln(regions.size() << " region(s)");

  // The pass brackets every region flush, so a screen can flush once for all
  // of them instead of once per region (and not at all when nothing changed).
  repaint_pass_begin();
  for (auto const &region : regions) {
    if (region.rect.empty()) {
      continue;
    }
    auto t0 = std::chrono::steady_clock::now();
    repaint_region(region.rect);
    auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    log_repaint_ln((region.source ? region.source->to_string() : std::string { "screen" }) << ": region (" << region.rect.x << ", " << region.rect.y << " " << region.rect.width << "x" << region.rect.height << ") took " << ms << " ms");
  }
  repaint_pass_end();
}

void Screen::repaint_if_due() {
  if (this->repaint_event_pending and std::chrono::steady_clock::now() >= this->repaint_due) {
    this->repaint_damaged();
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
  // Dispatching an event can unregister a listener: a window's mouse
  // dispatcher removes itself from the screen the moment the pointer leaves
  // that window (e.g. when it crosses from the menu bar into a popup, which
  // happens right here, inside this loop). Erasing the entry being visited
  // invalidates the traversal, so iterate over a snapshot instead. Holding
  // the shared pointers also keeps a listener that removed itself alive for
  // the remainder of this dispatch.
  auto listeners = std::vector<std::pair<EventTypeMask, std::shared_ptr<EventListener<Event>>>> { };
  listeners.reserve(this->selective_listeners.size());
  for (auto &&entry : this->selective_listeners) {
    listeners.emplace_back(entry.event_mask, entry.listener);
  }

  for (auto &&[event_mask, listener] : listeners) {
    if (event_mask & e.id) {
      listener->event_dispatched(e);
    }
  }
}

}
