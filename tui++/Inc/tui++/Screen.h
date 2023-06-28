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
  struct SelectiveEventListener {
    std::shared_ptr<EventListener<Event>> event_listener;
    EventTypeMask event_mask;
  };

protected:
  std::list<SelectiveEventListener> selective_event_listeners;

  bool quit = false;
  EventQueue event_queue;

  mutable std::mutex windows_mutex;
  std::list<std::shared_ptr<Window>> windows;

  Dimension size { };

private:
  void show_window(const std::shared_ptr<Window> &window);
  void hide_window(const std::shared_ptr<Window> &window);

  void to_front(const std::shared_ptr<Window> &window);
  void focus(const std::shared_ptr<Window> &window);

  friend class Window;

protected:
  Screen() = default;

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
    return this->event_queue;
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

  void post(const std::shared_ptr<Event> &event);

  template<typename T, typename Component, typename ... Args>
  void post(const std::shared_ptr<Component> &source, Args &&... args) {
    post(std::make_shared<T>(source, std::forward<Args>(args)...));
  }

  void post(std::function<void()> fn) {
    post(std::make_shared<InvocationEvent>(fn));
  }

  std::shared_ptr<Component> get_component_at(int x, int y) const;
  std::shared_ptr<Component> get_component_at(const Point &p) const {
    return get_component_at(p.x, p.y);
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

  void add_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener, const EventTypeMask &event_mask) {
    for (auto i = this->selective_event_listeners.begin(); i != this->selective_event_listeners.end(); ++i) {
      if (i->event_listener == event_listener) {
        i->event_mask |= event_mask;
        return;
      }
    }
    this->selective_event_listeners.emplace_back(event_listener, event_mask);
  }

  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto i = this->selective_event_listeners.begin(); i != this->selective_event_listeners.end();) {
      if (i->event_listener == event_listener) {
        this->selective_event_listeners.erase(i);
        break;
      } else {
        ++i;
      }
    }
  }

  void notify_event_listeners(Event &e) {
    for (auto &&selective_event_listener : this->selective_event_listeners) {
      if (selective_event_listener.event_mask & e.id) {
        selective_event_listener.event_listener->event_dispatched(e);
      }
    }
  }
};

}
