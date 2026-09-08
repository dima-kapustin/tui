#pragma once

#include <tui++/Event.h>
#include <tui++/Rectangle.h>
#include <tui++/EventQueue.h>

#include <list>
#include <mutex>
#include <vector>

namespace tui {

class Screen;
extern Screen &screen;

class Frame;
class Dialog;
class Window;
class Component;
class Graphics;
class Timer;

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

  // The pending Timer firings, in no particular order. Timers register and
  // cancel themselves here (Timer::start/stop); run_pending_timers fires the
  // due entries and re-queues the repeating ones. Only touched on the event
  // dispatch thread.
  struct PendingTimer {
    std::chrono::steady_clock::time_point due;
    Timer *timer;
  };
  std::vector<PendingTimer> timers;

  friend class Timer;

  void add_timer(Timer *timer);
  void remove_timer(Timer *timer);

  std::list<SelectiveListener> selective_listeners;

  bool quit = false;

  // Outstanding repaint requests, in screen coordinates (Swing's
  // RepaintManager keeps the same kind of dirty-region list). Regions are
  // kept separate when they cover very different parts of the screen and
  // merged when doing so is cheap, so a repaint pass does not flush one huge
  // bounding box that mostly contains unchanged content.
  struct DamagedRegion {
    Rectangle rect;

    // The component whose repaint() request recorded this damage, when the
    // request bubbled up through the component tree (a repaint requested
    // straight on the screen has none). Logged when the region is flushed so
    // the log says what is being repainted and why.
    std::shared_ptr<Component> source;
  };
  std::vector<DamagedRegion> damaged_regions;

  // True while a repaint invocation is queued: damage that accumulates until
  // that invocation runs is painted together, at most once per queue pass.
  bool repaint_event_pending = false;

  // When non-zero, repaint requests ride a frame clock instead of an
  // immediate queue invocation: the repaint runs at the next interval
  // boundary after the first damage, so a burst of input (fast mouse motion)
  // paints once -- the latest state -- instead of once per input batch. The
  // event loops set this; tests drive repaints directly and keep the
  // immediate (zero) behavior.
  std::chrono::milliseconds repaint_interval { };
  std::chrono::steady_clock::time_point repaint_due { };

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

  // Called around a repaint pass (the flush of all accumulated damaged
  // regions). Screens whose region flush is expensive use these to batch:
  // TextScreen defers its terminal flush to the end of the pass and skips it
  // entirely when no cell changed (see TextScreen::repaint_pass_end).
  virtual void repaint_pass_begin() {
  }

  virtual void repaint_pass_end() {
  }

public:
  EventQueue& get_event_queue() {
    return event_queue;
  }

  // Fires the timers whose time has come; the event loops call this on every
  // iteration, so timers keep firing while the loop idles. Callbacks run on
  // the calling (event dispatch) thread. Public so tests can drive timers
  // without running a full event loop.
  void run_pending_timers();

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

  // Records a repaint request for `rect` (screen coordinates) and schedules a
  // single repaint invocation on the event queue (Swing's
  // RepaintManager.addDirtyRegion + scheduled paint). Repainting then happens
  // on the dispatch thread, ordered with the mouse/key events that caused the
  // damage, and all damage that accumulates until the invocation runs is
  // painted together.
  void add_damage(Rectangle const &rect);

  // Records a repaint request on behalf of `source` -- the component whose
  // repaint() call caused the damage -- so the repaint log can say what is
  // being repainted.
  void add_damage(Rectangle const &rect, std::shared_ptr<Component> const &source);

  void add_damage(int x, int y, int width, int height) {
    add_damage(Rectangle { x, y, width, height });
  }

  void add_damage(int x, int y, int width, int height, std::shared_ptr<Component> const &source) {
    add_damage(Rectangle { x, y, width, height }, source);
  }

  // Repaints the accumulated damaged regions now and clears them. Called by
  // the queued repaint invocation; a no-op when nothing is damaged.
  void repaint_damaged();

  // Runs the pending repaint once its frame boundary is due. The event loops
  // call this after every input batch (see repaint_interval).
  void repaint_if_due();

  // Notifies the screen that the terminal was resized. Screens that poll the
  // size themselves (e.g. the pixel-level screens) may leave this empty.
  virtual void resized() {
  }

  void add_listener(const EventTypeMask &event_mask, const std::shared_ptr<EventListener<Event>> &listener);
  void remove_listener(const std::shared_ptr<EventListener<Event>> &listener);
  void notify_listeners(Event &e);
};

}
