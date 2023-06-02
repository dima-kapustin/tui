#include <chrono>
#include <iostream>

#include <tui++/Screen.h>
#include <tui++/Window.h>
#include <tui++/Terminal.h>

namespace tui {

bool Screen::quit = false;
EventQueue Screen::event_queue;
std::shared_ptr<Event> Screen::last_focus_event;
std::vector<std::shared_ptr<Window>> Screen::windows;
std::mutex Screen::mutex;

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

void Screen::run_event_loop() {
  while (not quit) {
    Terminal::read_events();
    if (auto event = event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      std::cout << *event << std::endl;
    }
  }
}

void Screen::post(const std::shared_ptr<Event> &event) {
  event_queue.push(event);
  if (event->type == Event::FOCUS) {
    last_focus_event = event;
  }
}

void Screen::post(std::function<void()> fn) {
  post(std::make_shared<Event>(fn));
}

std::shared_ptr<Window> Screen::get_top_window() {
  return windows.empty() ? nullptr : windows.back();
}

std::shared_ptr<Component> Screen::get_component_at(int x, int y) {
  auto top_window = get_top_window();
  auto component = top_window->get_child_at(x - top_window->get_x(), y - top_window->get_y());
  return component ? component : top_window;
}

void Screen::add_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(mutex);
  if (windows.empty()) {
    window->set_size(Terminal::get_size());
  }
  windows.emplace_back(window);
}

void Screen::remove_window(const std::shared_ptr<Window> &window) {
  std::unique_lock lock(mutex);
  if (auto pos = std::remove(windows.begin(), windows.end(), window); pos != windows.end()) {
    windows.erase(pos, windows.end());
  } else {
    throw std::runtime_error("unknown window");
  }
}

}
