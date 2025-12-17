#include <tui++/EventQueue.h>

namespace tui {
void EventQueue::push(const std::shared_ptr<Event> &event) {
  std::unique_lock lock(this->mutex);
  this->queue.emplace_back(event);
  lock.unlock();
  this->queue_cv.notify_one();
}

std::shared_ptr<Event> EventQueue::pop() {
  std::unique_lock lock(this->mutex);
  this->queue_cv.wait(lock, [this] {
    return not this->queue.empty();
  });
  auto event = std::move(this->queue.front());
  this->queue.pop_front();
  set_current_event(event);
  return event;
}

std::shared_ptr<Event> EventQueue::pop(const std::chrono::milliseconds &timeout) {
  std::unique_lock lock(this->mutex);
  if (this->queue.empty() and not this->queue_cv.wait_for(lock, timeout, [this] {
    return not this->queue.empty();
  })) {
    return {};
  }
  auto event = std::move(this->queue.front());
  this->queue.pop_front();
  set_current_event(event);
  return event;
}

void EventQueue::set_current_event(std::shared_ptr<Event> const &event) {
  this->current_event = event;
  this->most_recent_event_time = std::max(this->most_recent_event_time.load(), event->when);
  if (auto key_event = std::dynamic_pointer_cast<KeyEvent>(event)) {
    this->most_recent_key_event_time = event->when;
  }
}

}
