#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <algorithm>
#include <type_traits>

#include <tui++/Event.h>

#include <tui++/event/EventListener.h>
#include <tui++/event/EventDispatcher.h>
#include <tui++/event/event_traits.h>

namespace tui {

namespace detail {

template<typename Event, typename = void>
struct has_type_enum: std::false_type {
};

template<typename Event>
struct has_type_enum<Event, std::enable_if_t<std::is_enum_v<typename Event::Type>>> : std::true_type {
};

template<typename Event>
constexpr bool has_type_enum_v = has_type_enum<Event>::value;

template<typename Callable, typename Event>
constexpr bool is_global_function_v = std::is_convertible_v<Callable, void (*)(Event&)>;

template<typename >
class SingleEventSource;

template<typename Event>
class BasicFunctionalEventAdapter {
protected:
  FunctionalEventListener<Event> event_listener;

  template<typename >
  friend class SingleEventSource;

public:
  BasicFunctionalEventAdapter(FunctionalEventListener<Event> &&event_listener) :
      event_listener(std::move(event_listener)) {
  }

  BasicFunctionalEventAdapter(const FunctionalEventListener<Event> &event_listener) :
      event_listener(event_listener) {
  }

  BasicFunctionalEventAdapter(const BasicFunctionalEventAdapter&) = default;
  BasicFunctionalEventAdapter(BasicFunctionalEventAdapter&&) = default;
  BasicFunctionalEventAdapter& operator=(const BasicFunctionalEventAdapter&) = default;
  BasicFunctionalEventAdapter& operator=(BasicFunctionalEventAdapter&&) = default;

  void operator()(Event &e) const {
    this->event_listener(e);
  }

  constexpr bool operator==(const BasicFunctionalEventAdapter &other) const {
    return false;
  }
};

template<typename Event, std::enable_if_t<has_type_enum_v<Event>, bool> = true>
class FunctionalEventAdapter: public BasicFunctionalEventAdapter<Event> {
  std::vector<typename Event::Type> event_ids;

  template<typename >
  friend class SingleEventSource;

public:
  FunctionalEventAdapter(std::vector<typename Event::Type> &&event_types, FunctionalEventListener<Event> &&event_listener) :
      BasicFunctionalEventAdapter<Event>(std::move(event_listener)), event_ids(std::move(event_types)) {
  }

  FunctionalEventAdapter(std::vector<typename Event::Type> &&event_types, const FunctionalEventListener<Event> &event_listener) :
      BasicFunctionalEventAdapter<Event>(event_listener), event_ids(std::move(event_types)) {
  }

  FunctionalEventAdapter(const FunctionalEventAdapter&) = default;
  FunctionalEventAdapter(FunctionalEventAdapter&&) = default;
  FunctionalEventAdapter& operator=(const FunctionalEventAdapter&) = default;
  FunctionalEventAdapter& operator=(FunctionalEventAdapter&&) = default;

  void operator()(Event &e) const {
    if (std::ranges::contains(this->event_ids, e.id)) {
      this->event_listener(e);
    }
  }

  constexpr bool operator==(const FunctionalEventAdapter &other) const {
    return false;
  }
};

template<typename Event, typename = void>
struct event_listener_variant;

template<typename Event>
struct event_listener_variant<Event, std::enable_if_t<not has_type_enum_v<Event>>> {
  using type = std::variant<std::shared_ptr<EventListener<Event>>, BasicFunctionalEventAdapter<Event>>;
};

template<typename Event>
struct event_listener_variant<Event, std::enable_if_t<has_type_enum_v<Event>>> {
  using type = std::variant<std::shared_ptr<EventListener<Event>>, BasicFunctionalEventAdapter<Event>, FunctionalEventAdapter<Event>>;
};

template<typename Event>
using event_listener_variant_t = typename event_listener_variant<Event>::type;

template<typename ... Events>
class MultipleEventSource;

template<typename Event>
class SingleEventSource {
  std::vector<event_listener_variant_t<Event>> event_listeners;

  template<typename ... Events>
  friend class MultipleEventSource;

protected:
  virtual void process_event(Event &e) {
    for (auto &&event_listener : this->event_listeners) {
      std::visit([&e](auto &&event_listener) {
        using T = std::decay_t<decltype(event_listener)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<EventListener<Event>>>) {
          EventDispatcher::dispatch_event(event_listener, e);
        } else if constexpr (std::is_same_v<T, BasicFunctionalEventAdapter<Event>>) {
          event_listener(e);
        } else if constexpr (std::is_same_v<T, FunctionalEventAdapter<Event>>) {
          event_listener(e);
        }
      }, event_listener);
    }
  }

protected:
  constexpr bool add_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto &&l : this->event_listeners) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&l)) {
        if (*event_listener_ptr == event_listener) {
          return false;
        }
      }
    }
    this->event_listeners.emplace_back(event_listener);
    return true;
  }

  template<typename Callable>
  requires (std::is_invocable_v<Callable, Event&>)
  constexpr bool add_event_listener(Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<BasicFunctionalEventAdapter<Event>>(&l)) {
        if (auto target = adapter->event_listener.template target<std::decay_t<Callable>>()) {
          if constexpr (is_global_function_v<Callable, Event>) {
            if (*target == callable) {
              return false;
            }
          } else {
            return false;
          }
        }
      }
    }
    this->event_listeners.emplace_back(std::in_place_type<BasicFunctionalEventAdapter<Event>>, std::forward<Callable>(callable));
    return true;
  }

  template<typename Callable, typename E = Event>
  requires (std::is_invocable_v<Callable, E&> and has_type_enum_v<E>)
  constexpr bool add_event_listener(std::vector<typename E::Type> &&event_ids, Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<FunctionalEventAdapter<E>>(&l)) {
        if (auto target = adapter->event_listener.template target<std::decay_t<Callable>>()) {
          if constexpr (is_global_function_v<Callable, E>) {
            if (*target == callable) {
              return false;
            }
          } else {
            for (auto &&event_type : event_ids) {
              if (not std::ranges::contains(adapter->event_ids, event_type)) {
                adapter->event_ids.emplace_back(event_type);
              }
            }
            return false;
          }
        }
      }
    }
    this->event_listeners.emplace_back(std::in_place_type<FunctionalEventAdapter<E>>, std::move(event_ids), std::forward<Callable>(callable));
    return true;
  }

  constexpr bool remove_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end(); ++l) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&*l)) {
        if (*event_listener_ptr == event_listener) {
          this->event_listeners.erase(l);
          return true;
        }
      }
    }
    return false;
  }

  template<typename Callable>
  constexpr bool remove_event_listener(const Callable &callable) {
    auto result = false;
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<BasicFunctionalEventAdapter<Event>>(&*l)) {
        if (adapter->event_listener.template target<std::decay_t<Callable>>()) {
          l = this->event_listeners.erase(l);
          result = true;
          continue;
        }
      }
      ++l;
    }
    return result;
  }

  template<typename Callable, typename E = Event>
  requires (has_type_enum_v<E> )
  constexpr bool remove_event_listener(const std::vector<typename E::Type> &types, const Callable &callable) {
    auto result = false;
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<FunctionalEventAdapter<E>>(&*l)) {
        if (adapter->event_listener.template target<std::decay_t<Callable>>()) {
          if (types.empty() or adapter->event_ids.empty()) {
            l = this->event_listeners.erase(l);
            result = true;
            continue;
          } else {
            auto end = adapter->event_ids.end();
            for (auto &&type : types) {
              end = std::remove(adapter->event_ids.begin(), end, type);
            }
            adapter->event_ids.erase(end, adapter->event_ids.end());
            if (adapter->event_ids.empty()) {
              l = this->event_listeners.erase(l);
              result = true;
              continue;
            }
          }
        }
      }
      ++l;
    }
    return result;
  }
};

template<typename T, typename ... Types>
constexpr bool is_one_of_v = std::disjunction<std::is_same<T, Types> ...>::value;

class MultipleEventSourceBase {
  EventTypeMask event_listener_mask = EventTypeMask::NONE;

  template<typename ... Events>
  friend class MultipleEventSource;

public:
  constexpr bool has_event_listeners(EventType event_type) const {
    return bool(this->event_listener_mask & event_type);
  }

  constexpr bool has_event_listeners(EventTypeMask event_mask) const {
    return (this->event_listener_mask & event_mask) == event_mask;
  }

  constexpr EventTypeMask get_event_listener_mask() const {
    return this->event_listener_mask;
  }
};

template<typename ... Events>
class MultipleEventSource: virtual public MultipleEventSourceBase, virtual protected SingleEventSource<Events> ... {
  template<typename Event>
  constexpr void update_event_listener_mask() {
    auto event_mask = event_mask_v<Event>;
    if (SingleEventSource<Event>::event_listeners.empty()) {
      this->event_listener_mask &= ~event_mask;
      event_listener_mask_updated(event_mask, EventTypeMask::NONE);
    } else {
      this->event_listener_mask |= event_mask;
      event_listener_mask_updated(EventTypeMask::NONE, event_mask);
    }
  }

  virtual void event_listener_mask_updated(const EventTypeMask &removed, const EventTypeMask &added) {
  }

  template<typename E, typename ... Es>
  void process_event(Event &e) {
    if (event_type_v<E> == e.id.type) {
      SingleEventSource<E>::process_event(*dynamic_cast<E*>(&e));
    } else if constexpr (sizeof...(Es)) {
      process_event<Es...>(e);
    }
  }

protected:
  void process_event(Event &e) {
    process_event<Events...>(e);
  }

  using SingleEventSource<Events>::process_event...;

  template<typename Event>
  requires (is_one_of_v<Event, Events...> )
  constexpr size_t get_event_listener_count() const {
    return SingleEventSource<Event>::event_listeners.size();
  }

public:

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (is_one_of_v<Event, Events...> and std::is_convertible_v<Listener*, EventListener<Event>*>)
  constexpr void add_event_listener(const std::shared_ptr<Listener> &listener) {
    if (SingleEventSource<Event>::add_event_listener(std::static_pointer_cast<EventListener<Event>>(listener))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&>)
  constexpr void add_event_listener(Callable &&callable) {
    if (SingleEventSource<Event>::add_event_listener(std::forward<Callable>(callable))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void add_event_listener(typename Event::Type type, Callable &&callable) {
    if (SingleEventSource<Event>::add_event_listener(std::vector<typename Event::Type> {1, type}, std::forward<Callable>(callable))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void add_event_listener(std::initializer_list<typename Event::Type> types, Callable &&callable) {
    if (SingleEventSource<Event>::add_event_listener(std::vector<typename Event::Type> {types.begin(), types.end()}, std::forward<Callable>(callable))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (is_one_of_v<Event, Events...> and std::is_convertible_v<Listener*, EventListener<Event>*>)
  constexpr void remove_event_listener(const std::shared_ptr<Listener> &listener) {
    if (SingleEventSource<Event>::remove_event_listener(std::static_pointer_cast<EventListener<Event>>(listener))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&>)
  constexpr void remove_event_listener(const Callable &callable) {
    if (SingleEventSource<Event>::remove_event_listener(callable)) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void remove_event_listener(typename Event::Type type, const Callable &callable) {
    if (SingleEventSource<Event>::remove_event_listener(std::vector<typename Event::Type> {1, type}, callable)) {
      update_event_listener_mask<Event>();
    }
  }
};

}

template<typename ...Events>
using EventSource = detail::MultipleEventSource<Events...>;

template<typename BaseEventSource, typename ...Events>
class EventSourceExtension: public BaseEventSource, EventSource<Events...> {
protected:
  using BaseEventSource::get_event_listener_count;
  using EventSource<Events...>::get_event_listener_count;

public:
  using BaseEventSource::add_event_listener;
  using BaseEventSource::remove_event_listener;

  using EventSource<Events...>::add_event_listener;
  using EventSource<Events...>::remove_event_listener;
};

}
