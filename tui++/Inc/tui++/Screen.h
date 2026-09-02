#pragma once

#include <tui++/Event.h>
#include <tui++/Rectangle.h>
#include <tui++/EventQueue.h>

#include <list>
#include <mutex>

namespace tui {

class Screen;
extern Screen &screen;

class Frame;
class Dialog;
class Window;
class Graphics;

namespace laf {
class LookAndFeel;
}
class TextMetrics;

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

  mutable std::recursive_mutex windows_mutex;
  std::list<std::shared_ptr<Window>> windows;

  Dimension size { };

private:
  void show_window(const std::shared_ptr<Window> &window);
  void hide_window(const std::shared_ptr<Window> &window);

  void to_front(const std::shared_ptr<Window> &window);

  void focus(const std::shared_ptr<Window> &gained, const std::shared_ptr<Window> &lost);

  friend class Window;
  friend class Terminal;

protected:
  Screen() = default;
  Screen(Screen const&) = delete;
  Screen(Screen&&) = delete;

  virtual ~Screen() {
  }

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
  virtual std::unique_ptr<Graphics> get_graphics() = 0;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const& clip) = 0;

  // The look-and-feel (and text metrics) installed by this screen.
  virtual std::shared_ptr<laf::LookAndFeel> get_look_and_feel() const = 0;
  virtual std::shared_ptr<TextMetrics> get_text_metrics() const = 0;

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

  // Converts a terminal mouse position (reported in text cells) into this
  // screen's coordinate system: identity for the text screen, whose layout is
  // measured in cells; pixels for the graphic (sixel) screen, whose layout is
  // measured in pixels. The converted point is used for window hit-testing
  // and event retargeting, so both backends stay unit-consistent.
  virtual Point convert_mouse_point(int x, int y) const {
    return { x, y };
  }

  virtual void refresh() = 0;

  // Repaints and flushes only `rect` (screen coordinates). The default
  // implementation repaints everything; pixel-level screens repaint just the
  // damaged region so small edits do not re-encode the whole image.
  virtual void repaint_region(Rectangle const &rect) {
    refresh();
  }

  // Notifies the screen that the terminal was resized. Screens that poll the
  // size themselves (e.g. the pixel-level screens) may leave this empty.
  virtual void resized() {
  }

  void add_listener(const EventTypeMask &event_mask, const std::shared_ptr<EventListener<Event>> &listener);
  void remove_listener(const std::shared_ptr<EventListener<Event>> &listener);
  void notify_listeners(Event &e);
};

}
