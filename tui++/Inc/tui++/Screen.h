#pragma once

#include <mutex>
#include <vector>

#include <tui++/Point.h>
#include <tui++/Dimension.h>
#include <tui++/EventQueue.h>

namespace tui {

class Window;
class Graphics;

class Screen {
protected:
  bool quit = false;
  EventQueue event_queue;

  std::mutex windows_mutex;
  std::vector<std::shared_ptr<Window>> windows;

  Dimension size { };

private:
  void add_window(const std::shared_ptr<Window> &window);
  void remove_window(const std::shared_ptr<Window> &window);

  friend class Window;

protected:
  Screen() = default;

  void post_system(const std::shared_ptr<Event> &event) {
    event->system_generated = true;
    post(event);
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
  void post(std::function<void()> fn) {
    post(std::make_shared<Event>(std::in_place_type<InvocationEvent>, fn));
  }

  std::shared_ptr<Window> get_active_window();
  std::shared_ptr<Component> get_component_at(int x, int y);
  std::shared_ptr<Component> get_component_at(const Point &p) {
    return get_component_at(p.x, p.y);
  }

  virtual void refresh() = 0;

  template<typename W, typename ... Args>
  std::enable_if_t<std::is_convertible_v<W*, Window*>, std::shared_ptr<W>> create_window(Args &&...args) {
    return std::shared_ptr<W> { new W(*this, std::forward<Args>(args)...) };
  }
};

}
