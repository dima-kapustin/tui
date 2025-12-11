#include <tui++/Screen.h>
#include <chrono>
#include <stdexcept>

#include <tui++/Window.h>
#include <tui++/KeyboardFocusManager.h>

namespace tui {

void Screen::paint(Graphics &g) {

}

void Screen::post(const std::shared_ptr<Event> &event) {
  this->event_queue.push(event);
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
  this->windows.emplace_front(window);
  if (this->windows.size() == 1) {
    focus(window, nullptr);
  }
}

void Screen::hide_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (auto pos = std::remove(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
    this->windows.erase(pos, this->windows.end());
  } else {
    throw std::runtime_error("unknown window");
  }
}

void Screen::to_front(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (this->windows.front() != window) {
    auto front = this->windows.front();
    if (auto pos = std::remove(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
      this->windows.erase(pos, this->windows.end());
      this->windows.emplace_front(window);
      focus(window, front);
    } else {
      throw std::runtime_error("unknown window");
    }
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
    event.source->dispatch_event(event);
  }
}

}
