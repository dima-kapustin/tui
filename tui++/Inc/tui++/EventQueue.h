#pragma once

#include <list>
#include <mutex>
#include <chrono>
#include <memory>
#include <condition_variable>

#include <tui++/Event.h>

namespace tui {

class Component;

class EventQueue {
  mutable std::mutex mutex;
  std::list<std::shared_ptr<Event>> queue;
  std::condition_variable queue_cv;

  std::atomic<std::weak_ptr<Event>> current_event;
  std::atomic<EventClock::time_point> most_recent_event_time;
  std::atomic<EventClock::time_point> most_recent_key_event_time;

public:
  EventQueue() = default;

  EventQueue(EventQueue const&) = delete;
  EventQueue(EventQueue&&) = delete;

  EventQueue& operator=(EventQueue const&) = delete;
  EventQueue& operator=(EventQueue&&) = delete;

public:
  void push(const std::shared_ptr<Event> &event);

  template<typename T, typename ... Args>
  void push(Args &&... args) {
    push(std::make_shared<Event>(std::in_place_type<T>, std::forward<Args>(args)...));
  }

  std::shared_ptr<Event> pop();
  std::shared_ptr<Event> pop(const std::chrono::milliseconds &timeout);

  bool empty() const {
    return this->queue.empty();
  }

  std::shared_ptr<Event> get_current_event() const {
    return this->current_event.load().lock();
  }

  EventClock::time_point get_most_recent_event_time() const {
    return this->most_recent_event_time;
  }

  EventClock::time_point get_most_recent_key_event_time() const {
    return this->most_recent_key_event_time;
  }

private:
  void set_current_event(std::shared_ptr<Event> const &event);
};

}
