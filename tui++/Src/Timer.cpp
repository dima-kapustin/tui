#include <tui++/Timer.h>
#include <tui++/Screen.h>

namespace tui {

Timer::Timer(std::chrono::milliseconds period, std::function<void()> &&tick) :
    period(period), tick(std::move(tick)) {
}

Timer::~Timer() {
  stop();
}

void Timer::start() {
  // Restart the period from now when the timer was already running.
  stop();
  this->running = true;
  screen.add_timer(this);
}

void Timer::stop() {
  if (this->running) {
    screen.remove_timer(this);
    this->running = false;
  }
}

}
