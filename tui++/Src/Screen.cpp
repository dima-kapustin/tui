#include <tui++/Screen.h>
#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

#include <stdexcept>

namespace tui {

void Screen::paint(Graphics &g) {
  std::unique_lock lock(this->windows_mutex);
  for (auto &&window : this->windows) {
    window->paint(g);
  }
}

std::shared_ptr<Window> Screen::get_window_at(int x, int y) const {
  std::unique_lock lock(this->windows_mutex);
  for (auto &&window : this->windows) {
    if (window->contains(x, y)) {
      return window;
    }
  }
  return {};
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
  if (event.id == InvocationEvent::INVOCATION) {
    static_cast<InvocationEvent&>(event).dispatch();
  } else if (event.source) {
    std::dynamic_pointer_cast<Component>(event.source)->dispatch_event(event);
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
