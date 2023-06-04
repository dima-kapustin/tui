#include <tui++/Screen.h>
#include <chrono>
#include <stdexcept>

#include <tui++/Window.h>
#include <tui++/Terminal.h>

namespace tui {

void Screen::post(const std::shared_ptr<Event> &event) {
  this->event_queue.push(event);
}

std::shared_ptr<Window> Screen::get_top_window() {
  std::unique_lock lock(windows_mutex);
  return windows.empty() ? nullptr : windows.back();
}

std::shared_ptr<Component> Screen::get_component_at(int x, int y) {
  if (auto top_window = get_top_window()) {
    auto component = top_window->get_child_at(x - top_window->get_x(), y - top_window->get_y());
    return component ? component : top_window;
  }
  return {};
}

void Screen::add_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (this->windows.empty()) {
    window->set_size(get_size());
  }
  this->windows.emplace_back(window);
}

void Screen::remove_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(this->windows_mutex);
  if (auto pos = std::remove(this->windows.begin(), this->windows.end(), window); pos != this->windows.end()) {
    this->windows.erase(pos, this->windows.end());
  } else {
    throw std::runtime_error("unknown window");
  }
}

void Screen::refresh() {

}

}
