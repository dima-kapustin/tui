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
    focus(window);
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
    if (auto pos = std::remove(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
      this->windows.erase(pos, this->windows.end());
      this->windows.emplace_front(window);
    } else {
      throw std::runtime_error("unknown window");
    }
  }
  focus(window);
}

void Screen::focus(const std::shared_ptr<Window> &window) {
  post_system<WindowEvent>(window, WindowEvent::WINDOW_GAINED_FOCUS);
}

void Screen::dispatch_event(Event &event) {
  if (event.id == InvocationEvent::INVOCATION) {
    static_cast<InvocationEvent&>(event).dispatch();
  } else if (event.source) {
    event.source->dispatch_event(event);
  }
}

}
