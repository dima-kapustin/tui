#pragma once

// Timer - a repeating timer that fires its callback on the event-dispatch
// thread, Swing's javax.swing.Timer analog. The screen's event loops run due
// timers every iteration (see Screen::run_pending_timers), so a timer keeps
// firing while the loop idles; callbacks must therefore not block.
//
// Timers are single-threaded: start/stop must be called from the event
// dispatch thread (components do this from their event handlers, layout and
// paint code), and the callback always runs there too.

#include <chrono>
#include <functional>

namespace tui {

class Timer {
public:
  using Clock = std::chrono::steady_clock;

  // `period` between firings while running; `tick` is invoked on the event
  // dispatch thread.
  Timer(std::chrono::milliseconds period, std::function<void()> &&tick);

  ~Timer();

  Timer(Timer const&) = delete;
  Timer& operator=(Timer const&) = delete;

  // Starts (or restarts, from now) the periodic firing; a no-op when the
  // timer is already running.
  void start();

  // Cancels pending firings; the timer may be started again.
  void stop();

  // Sets the firing period; the next firing uses the new value.
  void set_period(std::chrono::milliseconds period) {
    this->period = period;
  }

  bool is_running() const {
    return this->running;
  }

private:
  friend class Screen;

  std::chrono::milliseconds period;
  std::function<void()> tick;
  bool running = false;
};

}
