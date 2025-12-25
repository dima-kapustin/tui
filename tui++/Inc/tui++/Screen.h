#pragma once

#include <list>
#include <mutex>

#include <tui++/Event.h>
#include <tui++/Point.h>
#include <tui++/Dimension.h>
#include <tui++/EventQueue.h>

namespace tui {

class Screen;
namespace detail {
Screen& get_screen();
}
static Screen& screen = detail::get_screen();

class Frame;
class Dialog;
class Window;
class Graphics;

class Screen {
  struct SelectiveListener {
    EventTypeMask event_mask;
    std::shared_ptr<EventListener<Event>> listener;
  };

protected:
  std::thread::id event_dispatching_thread_id;
  EventQueue event_queue;

  std::list<SelectiveListener> selective_listeners;

  bool quit = false;

  mutable std::mutex windows_mutex;
  std::list<std::shared_ptr<Window>> windows;

  Dimension size { };

private:
  void show_window(const std::shared_ptr<Window> &window);
  void hide_window(const std::shared_ptr<Window> &window);

  void to_front(const std::shared_ptr<Window> &window);

  void focus(const std::shared_ptr<Window> &gained, const std::shared_ptr<Window> &lost);

  friend class Window;

protected:
  Screen() = default;
  Screen(Screen const&) = delete;
  Screen(Screen&&) = delete;

  Screen& operator=(Screen const&) = delete;
  Screen& operator=(Screen&&) = delete;

  void post_system(const std::shared_ptr<Event> &event) {
    event->system_generated = true;
    post(event);
  }

  template<typename T, typename Component, typename ... Args>
  void post_system(const std::shared_ptr<Component> &source, Args &&... args) {
    post_system(std::make_shared<T>(source, std::forward<Args>(args)...));
  }

  void paint(Graphics &g);

  void dispatch_event(Event &event);

public:
  EventQueue& get_event_queue() {
    return event_queue;
  }

  /**
   * @return true iff the calling thread is the event dispatching thread
   */
  bool is_event_dispatching_thread() {
    return std::this_thread::get_id() == event_dispatching_thread_id;
  }

  virtual void run_event_loop() = 0;

  int get_width() const {
    return this->size.width;
  }

  int get_height() const {
    return this->size.height;
  }

  const Dimension& get_size() const {
    return this->size;
  }

  void post(const std::shared_ptr<Event> &event) {
    event_queue.push(event);
  }

  template<typename T, typename Component, typename ... Args>
  void post(const std::shared_ptr<Component> &source, Args &&... args) {
    post(std::make_shared<T>(source, std::forward<Args>(args)...));
  }

  void post(std::function<void()> fn) {
    post(std::make_shared<InvocationEvent>(fn));
  }

  std::shared_ptr<Window> get_window_at(int x, int y) const;
  std::shared_ptr<Window> get_window_at(const Point &p) const {
    return get_window_at(p.x, p.y);
  }

  virtual void refresh() = 0;

  void add_listener(const EventTypeMask &event_mask, const std::shared_ptr<EventListener<Event>> &listener);
  void remove_listener(const std::shared_ptr<EventListener<Event>> &listener);
  void notify_listeners(Event &e);
};

}
