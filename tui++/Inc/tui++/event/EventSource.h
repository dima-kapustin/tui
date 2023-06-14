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

template<typename Event>
class BasicFunctionalEventAdapter: public EventListener<Event> {
protected:
  FunctionalEventListener<Event> event_listener;

  template<typename >
  friend class EventSource;

public:
  BasicFunctionalEventAdapter(FunctionalEventListener<Event> &&event_listener) :
      event_listener(std::move(event_listener)) {
  }

  BasicFunctionalEventAdapter(const FunctionalEventListener<Event> &event_listener) :
      event_listener(event_listener) {
  }

  void on_event(Event &e) override {
    this->event_listener(e);
  }
};

template<typename Event, std::enable_if_t<has_type_enum_v<Event>, bool> = true>
class FunctionalEventAdapter: public BasicFunctionalEventAdapter<Event> {
  std::vector<typename Event::Type> event_types;

  template<typename >
  friend class EventSource;

public:
  FunctionalEventAdapter(std::vector<typename Event::Type> &&event_types, FunctionalEventListener<Event> &&event_listener) :
      BasicFunctionalEventAdapter<Event>(std::move(event_listener)), event_types(std::move(event_types)) {
  }

  FunctionalEventAdapter(std::vector<typename Event::Type> &&event_types, const FunctionalEventListener<Event> &event_listener) :
      BasicFunctionalEventAdapter<Event>(event_listener), event_types(std::move(event_types)) {
  }

  void on_event(Event &e) override {
    if (std::ranges::contains(this->event_types, e.type)) {
      this->event_listener(e);
    }
  }
};

template<typename Event>
class EventSource {
  std::vector<std::shared_ptr<EventListener<Event>>> event_listeners;

protected:
  virtual void process_event(Event &e) {
    for (auto &&l : this->event_listeners) {
      l->on_event(e);
    }
  }

protected:
  void add_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    if (not std::ranges::contains(this->event_listeners, event_listener)) {
      this->event_listeners.emplace_back(event_listener);
    }
  }

  void add_event_listener(FunctionalEventListener<Event> &&event_listener) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::dynamic_pointer_cast<BasicFunctionalEventAdapter<Event>>(l)) {
        if (adapter->event_listener.template target<void(Event &e)>() == event_listener.template target<void(Event &e)>()) {
          return;
        }
      }
    }
    add_event_listener(std::make_shared<BasicFunctionalEventAdapter<Event>>(std::move(event_listener)));
  }

  template<typename E = Event>
  std::enable_if_t<has_type_enum_v<E>, void> add_event_listener(std::vector<typename E::Type> &&event_types, FunctionalEventListener<E> &&event_listener) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::dynamic_pointer_cast<FunctionalEventAdapter<E>>(l)) {
        if (adapter->event_listener.template target<void(E &e)>() == event_listener.template target<void(E &e)>()) {
          for (auto &&event_type : event_types) {
            if (not std::ranges::contains(adapter->event_types, event_type)) {
              adapter->event_types.emplace_back(event_type);
            }
          }
          return;
        }
      }
    }
    add_event_listener(std::make_shared<FunctionalEventAdapter<E>>(std::move(event_types), std::move(event_listener)));
  }

  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end(); ++l) {
      if (*l == event_listener) {
        this->event_listeners.erase(l);
        return;
      }
    }
  }

  void remove_event_listener(const FunctionalEventListener<Event> &event_listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::dynamic_pointer_cast<BasicFunctionalEventAdapter<Event>>(*l)) {
        if (adapter->event_listener.template target<void(Event &e)>() == event_listener.template target<void(Event &e)>()) {
          l = this->event_listeners.erase(l);
          continue;
        }
      }
      ++l;
    }
  }

  template<typename E = Event>
  std::enable_if_t<has_type_enum_v<E>, void> remove_event_listener(const std::vector<typename E::Type> &types, const FunctionalEventListener<E> &event_listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::dynamic_pointer_cast<FunctionalEventAdapter<E>>(*l)) {
        if (adapter->event_listener.template target<void(E &e)>() == event_listener.template target<void(E &e)>()) {
          if (types.empty() or adapter->event_types.empty()) {
            l = this->event_listeners.erase(l);
            continue;
          } else {
            auto end = adapter->event_types.end();
            for (auto &&type : types) {
              end = std::remove(adapter->event_types.begin(), end, type);
            }
            adapter->event_types.erase(end, adapter->event_types.end());
            if (adapter->event_types.empty()) {
              l = this->event_listeners.erase(l);
              continue;
            }
          }
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
class BasicEventSource: protected virtual detail::EventSource<Events> ... {
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
    detail::EventSource<Event>::add_event_listener(FunctionalEventListener<Event>(std::forward<Callable>(callable)));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<detail::has_type_enum_v<Event>, void> add_event_listener(typename Event::Type type, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { 1, type }, FunctionalEventListener<Event>(std::forward<Callable>(callable)));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<detail::has_type_enum_v<Event>, void> add_event_listener(std::initializer_list<typename Event::Type> types, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { types.begin(), types.end() }, FunctionalEventListener<Event>(std::forward<Callable>(callable)));
  }

  template<typename Event>
  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    detail::EventSource<Event>::remove_event_litener(listener);
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  void remove_event_listener(Callable &&callable) {
    detail::EventSource<Event>::remove_event_listener(FunctionalEventListener<Event>(std::forward<Callable>(callable)));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<detail::has_type_enum_v<Event>, void> remove_event_listener(typename Event::Type type, Callable &&callable) {
    detail::EventSource<Event>::remove_event_listener(std::vector<typename Event::Type> { 1, type }, FunctionalEventListener<Event>(std::forward<Callable>(callable)));
  }
};

}
