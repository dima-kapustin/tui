#pragma once

#include <vector>
#include <memory>
#include <type_traits>

namespace tui {

template<typename Event>
class EventListener {
public:
  virtual ~EventListener() {
  }

  virtual void on_event(Event &e) = 0;
};

template<typename Event>
using FunctionalEventListener = std::function<void(Event &e)>;

namespace detail {

template<typename Event, typename = void>
struct has_type_enum: std::false_type {
};

template<typename Event>
struct has_type_enum<Event, std::enable_if_t<std::is_enum_v<typename Event::Type>>> : std::true_type {
};

template<typename Event>
constexpr bool has_type_enum_v = has_type_enum<Event>::value;

template<typename >
class EventSource;

template<typename Event, std::enable_if_t<has_type_enum_v<Event>, bool> = true>
class FunctionalEventAdapter: public EventListener<Event> {
  std::vector<typename Event::Type> event_types;
  FunctionalEventListener<Event> listener;

  template<typename >
  friend class EventSource;

public:
  FunctionalEventAdapter(FunctionalEventListener<Event> &&listener) :
      listener(std::move(listener)) {
  }

  FunctionalEventAdapter(const FunctionalEventListener<Event> &listener) {
  }

  FunctionalEventAdapter(std::vector<typename Event::Type> &&event_types, const FunctionalEventListener<Event> &listener) :
      event_types(std::move(event_types)), listener(listener) {
  }

  virtual void on_event(Event &e) override {
    if (this->event_types.empty() or std::ranges::contains(this->event_types, e.type)) {
      this->listener(e);
    }
  }
};

template<typename Event>
class EventSource {
  std::vector<std::weak_ptr<EventListener<Event>>> listeners;

protected:
  virtual void process_event(Event &e) {
    for (auto l = this->listeners.begin(); l != this->listeners.end();) {
      if (auto listener = l->lock()) {
        listener->on_event(e);
        ++l;
      } else {
        this->listeners.erase(l);
      }
    }
  }

protected:
  void add_event_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    for (auto &&l : this->listeners) {
      if (l.lock() == listener) {
        return;
      }
    }
    this->listeners.emplace_back(listener);
  }

  void add_event_listener(const FunctionalEventListener<Event> &listener) {
    add_event_listener(std::make_shared<FunctionalEventAdapter<Event>>(std::vector<typename Event::Type> { }, listener));
  }

  std::enable_if_t<has_type_enum_v<Event>, void> add_event_listener(std::vector<typename Event::Type> &&types, const FunctionalEventListener<Event> &listener) {
    add_event_listener(std::make_shared<FunctionalEventAdapter<Event>>(std::move(types), listener));
  }

  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    for (auto l = this->listeners.begin(); l != this->listeners.end(); ++l) {
      if (l->lock() == listener) {
        this->listeners.erase(l);
        return;
      }
    }
  }

  std::enable_if_t<has_type_enum_v<Event>, void> remove_event_listener(const FunctionalEventListener<Event> &listener) {
    for (auto l = this->listeners.begin(); l != this->listeners.end();) {
      if (auto adapter = std::dynamic_pointer_cast<FunctionalEventAdapter<Event>>(l->lock())) {
        if (adapter->listener.template target<void(Event &e)>() == listener.template target<void(Event &e)>()) {
          l = this->listeners.erase(l);
          continue;
        }
      }
      ++l;
    }
  }

};

template<typename Callable, typename Event>
struct __event_type_from_callable: std::enable_if<std::is_convertible_v<Callable, FunctionalEventListener<Event>>, Event> {
};

template<typename Callable, typename ... Events>
struct event_type_from_callable: __event_type_from_callable<Callable, Events> ... {
};

template<typename Callable, typename ... Events>
using event_type_from_callable_t = typename event_type_from_callable<Callable, Events...>::type;

}

template<typename ... Events>
class BasicEventSource: protected detail::EventSource<Events> ... {
protected:
//
  using detail::EventSource<Events>::process_event...;

public:
  template<typename Event>
  void add_event_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    detail::EventSource<Event>::add_event_litener(listener);
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  void add_event_listener(Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(FunctionalEventListener<Event>(std::move(callable)));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  void add_event_listener(typename Event::Type type, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { 1, type }, FunctionalEventListener<Event>(std::move(callable)));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<detail::has_type_enum_v<Event>, void> add_event_listener(std::initializer_list<typename Event::Type> types, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { types.begin(), types.end() }, FunctionalEventListener<Event>(std::move(callable)));
  }

  template<typename Event>
  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    detail::EventSource<Event>::remove_event_litener(listener);
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<detail::has_type_enum_v<Event>, void> remove_event_listener(Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(FunctionalEventListener<Event>(std::move(callable)));
  }
};

}
