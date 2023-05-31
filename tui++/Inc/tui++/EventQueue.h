#pragma once

#include <list>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include <tui++/Event.h>

namespace tui {

class EventQueue {
  std::mutex mutex;
  std::list<std::unique_ptr<Event>> queue;
  std::condition_variable queue_cv;

public:
  void push(std::unique_ptr<Event> &&event) {
    std::unique_lock lock(this->mutex);
    this->queue.emplace_back(std::move(event));
    lock.unlock();
    this->queue_cv.notify_one();
  }

  std::unique_ptr<Event> pop() {
    std::unique_lock lock(this->mutex);
    this->queue_cv.wait(lock, [this] {
      return not this->queue.empty();
    });
    auto event = std::move(this->queue.front());
    this->queue.pop_front();
    return event;
  }

  std::unique_ptr<Event> pop(const std::chrono::milliseconds &timeout) {
    std::unique_lock lock(this->mutex);
    if (this->queue.empty() and not this->queue_cv.wait_for(lock, timeout, [this] {
      return not this->queue.empty();
    })) {
      return {};
    }
    auto event = std::move(this->queue.front());
    this->queue.pop_front();
    return event;
  }

  bool empty() const {
    return this->queue.empty();
  }
};

}
