#pragma once

#include <list>
#include <mutex>

#include <tui++/Event.h>
#include <tui++/Point.h>
#include <tui++/Dimension.h>
#include <tui++/EventQueue.h>

namespace tui {

class Frame;
class Dialog;
class Window;
class Graphics;

class Screen {
  struct SelectiveListener {
    std::shared_ptr<EventListener<Event>> listener;
    EventTypeMask event_mask;
  };

protected:
  static std::thread::id event_dispatching_thread_id;
  static EventQueue event_queue;

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
  static EventQueue& get_event_queue() {
    return event_queue;
  }

  /**
   * @return true iff the calling thread is the event dispatching thread
   */
  static bool is_event_dispatching_thread() {
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

  static void post(const std::shared_ptr<Event> &event) {
    event_queue.push(event);
  }

  template<typename T, typename Component, typename ... Args>
  static void post(const std::shared_ptr<Component> &source, Args &&... args) {
    post(std::make_shared<T>(source, std::forward<Args>(args)...));
  }

  static void post(std::function<void()> fn) {
    post(std::make_shared<InvocationEvent>(fn));
  }

  std::shared_ptr<Window> get_window_at(int x, int y) const;
  std::shared_ptr<Window> get_window_at(const Point &p) const {
    return get_window_at(p.x, p.y);
  }

  virtual void refresh() = 0;

  template<typename F, typename ... Args>
  requires (std::is_convertible_v<F*, Frame*>)
  std::shared_ptr<F> create_frame(Args &&...args) {
    auto frame = std::shared_ptr<F> { new F(*this, std::forward<Args>(args)...) };
    frame->init();
    return frame;
  }

  template<typename D, typename ... Args>
  requires (std::is_convertible_v<D*, Dialog*>)
  std::shared_ptr<D> create_dialog(Args &&...args) {
    auto dialog = std::shared_ptr<D> { new D(*this, std::forward<Args>(args)...) };
    dialog->init();
    return dialog;
  }

  void add_listener(const std::shared_ptr<EventListener<Event>> &listener, const EventTypeMask &event_mask) {
    for (auto i = this->selective_listeners.begin(); i != this->selective_listeners.end(); ++i) {
      if (i->listener == listener) {
        i->event_mask |= event_mask;
        return;
      }
    }
    this->selective_listeners.emplace_back(listener, event_mask);
  }

  void remove_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    for (auto i = this->selective_listeners.begin(); i != this->selective_listeners.end();) {
      if (i->listener == listener) {
        this->selective_listeners.erase(i);
        break;
      } else {
        ++i;
      }
    }
  }

  void notify_event_listeners(Event &e) {
    for (auto &&selective_listener : this->selective_listeners) {
      if (selective_listener.event_mask & e.id) {
        selective_listener.listener->event_dispatched(e);
      }
    }
  }
};

}

#include <tui++/Component.h>

namespace tui {

template<typename T, typename ... Args>
inline void Component::post_event(Args &&... args) {
  if (is_event_enabled<T>()) {
    Screen::post<T>(shared_from_this(), std::forward<Args>(args)...);
  }
}

template<typename T, typename DerivedComponent, typename ... Args>
inline void Component::post_event(Args &&... args) {
  if (is_event_enabled<T>()) {
    Screen::post<T>(std::dynamic_pointer_cast<DerivedComponent>(shared_from_this()), std::forward<Args>(args)...);
  }
}

}
