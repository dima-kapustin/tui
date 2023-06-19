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

public:
  EventQueue() = default;

  EventQueue(const EventQueue&) = delete;
  EventQueue(EventQueue&&) = delete;

  EventQueue& operator=(const EventQueue&) = delete;
  EventQueue& operator=(EventQueue&&) = delete;

public:
  void push(const std::shared_ptr<Event> &event) {
    std::unique_lock lock(this->mutex);
    this->queue.emplace_back(event);
    lock.unlock();
    this->queue_cv.notify_one();
  }

  template<typename T, typename ... Args>
  void push(Args &&... args) {
    push(std::make_shared<Event>(std::in_place_type<T>, std::forward<Args>(args)...));
  }

  std::shared_ptr<Event> pop() {
    std::unique_lock lock(this->mutex);
    this->queue_cv.wait(lock, [this] {
      return not this->queue.empty();
    });
    auto event = std::move(this->queue.front());
    this->queue.pop_front();
    return event;
  }

  std::shared_ptr<Event> pop(const std::chrono::milliseconds &timeout) {
    std::unique_lock lock(this->mutex);
    if (this->queue.empty() and not this->queue_cv.wait_for(lock, timeout, [this] {
      return not this->queue.empty();
    })) {
      return {};
    }
    auto event = std::move(this->queue.front());
    this->queue.pop_front();
    this->current_event = event;
    return event;
  }

  bool empty() const {
    return this->queue.empty();
  }

  std::shared_ptr<Event> get_current_event() const {
    return this->current_event.load().lock();
  }
};

}
