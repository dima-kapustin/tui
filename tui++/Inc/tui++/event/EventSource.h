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
class BasicFunctionalAdapter {
protected:
  FunctionalEventListener<Event> listener;

  template<typename >
  friend class SingleEventSource;

public:
  BasicFunctionalAdapter(FunctionalEventListener<Event> &&listener) :
      listener(std::move(listener)) {
  }

  BasicFunctionalAdapter(const FunctionalEventListener<Event> &listener) :
      listener(listener) {
  }

  BasicFunctionalAdapter(const BasicFunctionalAdapter&) = default;
  BasicFunctionalAdapter(BasicFunctionalAdapter&&) = default;
  BasicFunctionalAdapter& operator=(const BasicFunctionalAdapter&) = default;
  BasicFunctionalAdapter& operator=(BasicFunctionalAdapter&&) = default;

  void operator()(Event &e) const {
    this->listener(e);
  }

  constexpr bool operator==(const BasicFunctionalAdapter &other) const {
    return false;
  }
};

template<typename Event, std::enable_if_t<has_type_enum_v<Event>, bool> = true>
class FunctionalAdapter: public BasicFunctionalAdapter<Event> {
  std::vector<typename Event::Type> event_ids;

  template<typename >
  friend class SingleEventSource;

public:
  FunctionalAdapter(std::vector<typename Event::Type> &&event_types, FunctionalEventListener<Event> &&listener) :
      BasicFunctionalAdapter<Event>(std::move(listener)), event_ids(std::move(event_types)) {
  }

  FunctionalAdapter(std::vector<typename Event::Type> &&event_types, const FunctionalEventListener<Event> &listener) :
      BasicFunctionalAdapter<Event>(listener), event_ids(std::move(event_types)) {
  }

  FunctionalAdapter(const FunctionalAdapter&) = default;
  FunctionalAdapter(FunctionalAdapter&&) = default;
  FunctionalAdapter& operator=(const FunctionalAdapter&) = default;
  FunctionalAdapter& operator=(FunctionalAdapter&&) = default;

  void operator()(Event &e) const {
    if (std::ranges::contains(this->event_ids, e.id)) {
      this->listener(e);
    }
  }

  constexpr bool operator==(const FunctionalAdapter &other) const {
    return false;
  }
};

template<typename Event, typename = void>
struct event_listener_variant;

template<typename Event>
struct event_listener_variant<Event, std::enable_if_t<not has_type_enum_v<Event>>> {
  using type = std::variant<std::shared_ptr<EventListener<Event>>, BasicFunctionalAdapter<Event>>;
};

template<typename Event>
struct event_listener_variant<Event, std::enable_if_t<has_type_enum_v<Event>>> {
  using type = std::variant<std::shared_ptr<EventListener<Event>>, BasicFunctionalAdapter<Event>, FunctionalAdapter<Event>>;
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
  void fire_event(Event &e) {
    process_event(e);
  }

  virtual void process_event(Event &e) {
    for (auto &&listener : this->event_listeners) {
      std::visit([&e](auto &&listener) {
        using T = std::decay_t<decltype(listener)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<EventListener<Event>>>) {
          EventDispatcher::dispatch_event(listener, e);
        } else if constexpr (std::is_same_v<T, BasicFunctionalAdapter<Event>>) {
          listener(e);
        } else if constexpr (std::is_same_v<T, FunctionalAdapter<Event>>) {
          listener(e);
        }
      }, listener);
    }
  }

protected:
  constexpr bool add_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    for (auto &&l : this->event_listeners) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&l)) {
        if (*event_listener_ptr == listener) {
          return false;
        }
      }
    }
    this->event_listeners.emplace_back(listener);
    return true;
  }

  template<typename Callable>
  requires (std::is_invocable_v<Callable, Event&>)
  constexpr bool add_listener(Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<BasicFunctionalAdapter<Event>>(&l)) {
        if (auto target = adapter->listener.template target<std::decay_t<Callable>>()) {
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
    this->event_listeners.emplace_back(std::in_place_type<BasicFunctionalAdapter<Event>>, std::forward<Callable>(callable));
    return true;
  }

  template<typename Callable, typename E = Event>
  requires (std::is_invocable_v<Callable, E&> and has_type_enum_v<E>)
  constexpr bool add_listener(std::vector<typename E::Type> &&event_ids, Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<FunctionalAdapter<E>>(&l)) {
        if (auto target = adapter->listener.template target<std::decay_t<Callable>>()) {
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
    this->event_listeners.emplace_back(std::in_place_type<FunctionalAdapter<E>>, std::move(event_ids), std::forward<Callable>(callable));
    return true;
  }

  constexpr bool remove_listener(const std::shared_ptr<EventListener<Event>> &listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end(); ++l) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&*l)) {
        if (*event_listener_ptr == listener) {
          this->event_listeners.erase(l);
          return true;
        }
      }
    }
    return false;
  }

  template<typename Callable>
  constexpr bool remove_listener(const Callable &callable) {
    auto result = false;
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<BasicFunctionalAdapter<Event>>(&*l)) {
        if (adapter->listener.template target<std::decay_t<Callable>>()) {
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
  constexpr bool remove_listener(const std::vector<typename E::Type> &types, const Callable &callable) {
    auto result = false;
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<FunctionalAdapter<E>>(&*l)) {
        if (adapter->listener.template target<std::decay_t<Callable>>()) {
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
  void invoke_process_event(Event &e) {
    if (event_type_v<E> == e.id.type) {
      SingleEventSource<E>::process_event(*dynamic_cast<E*>(&e));
    } else if constexpr (sizeof...(Es)) {
      invoke_process_event<Es...>(e);
    }
  }

protected:
  void process_event(Event &e) {
    invoke_process_event<Events...>(e);
  }

  using SingleEventSource<Events>::process_event...;

  template<typename Event, typename ... Args>
  requires (is_one_of_v<Event, Events...> )
  void fire_event(Args &&...args) {
    auto e = Event { std::forward<Args>(args)... };
    process_event(e);
  }

  template<typename Event>
  requires (is_one_of_v<Event, Events...> )
  void fire_event(Event &e) {
    process_event(e);
  }

  template<typename Event>
  requires (is_one_of_v<Event, Events...> )
  constexpr size_t get_event_listener_count() const {
    return SingleEventSource<Event>::event_listeners.size();
  }

public:

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (is_one_of_v<Event, Events...> and std::is_convertible_v<Listener*, EventListener<Event>*>)
  constexpr void add_listener(const std::shared_ptr<Listener> &listener) {
    if (SingleEventSource<Event>::add_listener(std::static_pointer_cast<EventListener<Event>>(listener))) {
      if constexpr (has_type_enum_v<Event>) {
        update_event_listener_mask<Event>();
      }
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&>)
  constexpr void add_listener(Callable &&callable) {
    if (SingleEventSource<Event>::add_listener(std::forward<Callable>(callable))) {
      if constexpr (has_type_enum_v<Event>) {
        update_event_listener_mask<Event>();
      }
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void add_listener(typename Event::Type type, Callable &&callable) {
    if (SingleEventSource<Event>::add_listener(std::vector<typename Event::Type> {1, type}, std::forward<Callable>(callable))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void add_listener(std::initializer_list<typename Event::Type> types, Callable &&callable) {
    if (SingleEventSource<Event>::add_listener(std::vector<typename Event::Type> {types.begin(), types.end()}, std::forward<Callable>(callable))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Listener, typename Event = event_type_from_listener_t<Listener>>
  requires (is_one_of_v<Event, Events...> and std::is_convertible_v<Listener*, EventListener<Event>*>)
  constexpr void remove_listener(const std::shared_ptr<Listener> &listener) {
    if (SingleEventSource<Event>::remove_listener(std::static_pointer_cast<EventListener<Event>>(listener))) {
      update_event_listener_mask<Event>();
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&>)
  constexpr void remove_listener(const Callable &callable) {
    if (SingleEventSource<Event>::remove_listener(callable)) {
      if constexpr (has_type_enum_v<Event>) {
        update_event_listener_mask<Event>();
      }
    }
  }

  template<typename Callable, typename Event = event_type_from_callable_t<Callable, Events...>>
  requires (is_one_of_v<Event, Events...> and std::is_invocable_v<Callable, Event&> and has_type_enum_v<Event>)
  constexpr void remove_listener(typename Event::Type type, const Callable &callable) {
    if (SingleEventSource<Event>::remove_listener(std::vector<typename Event::Type> {1, type}, callable)) {
      update_event_listener_mask<Event>();
    }
  }
};

}

template<typename ...Events>
using EventSource = detail::MultipleEventSource<Events...>;

template<typename BaseEventSource, typename ...Events>
class EventSourceExtension: public BaseEventSource, EventSource<Events...> {
public:
  using BaseEventSource::add_listener;
  using BaseEventSource::remove_listener;

  using EventSource<Events...>::add_listener;
  using EventSource<Events...>::remove_listener;

protected:
  template<typename ... Args>
  EventSourceExtension(Args &&...args) :
      BaseEventSource(std::forward<Args>(args)...) {
  }

protected:
  using BaseEventSource::process_event;
  using EventSource<Events...>::process_event;

  using BaseEventSource::get_event_listener_count;
  using EventSource<Events...>::get_event_listener_count;

  using BaseEventSource::fire_event;
  using EventSource<Events...>::fire_event;
};

}
