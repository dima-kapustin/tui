#pragma once

#include <vector>
#include <memory>
#include <variant>
#include <algorithm>
#include <type_traits>

#include <tui++/Event.h>

#include <tui++/event/EventListener.h>

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

template<typename, typename = void>
struct has_type_member: std::false_type {
};

template<typename T>
struct has_type_member<T, std::void_t<typename T::type>> : std::true_type {
};

template<typename T>
constexpr bool has_type_member_v = has_type_member<T>::value;

template<typename Callable, typename Event>
constexpr bool is_global_function_v = std::is_convertible_v<Callable, void (*)(Event&)>;

template<typename >
class EventSource;

template<typename Event>
class BasicFunctionalEventAdapter {
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

  constexpr bool operator!=(const BasicFunctionalEventAdapter &other) const {
    return true;
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

  FunctionalEventAdapter(const FunctionalEventAdapter&) = default;
  FunctionalEventAdapter(FunctionalEventAdapter&&) = default;
  FunctionalEventAdapter& operator=(const FunctionalEventAdapter&) = default;
  FunctionalEventAdapter& operator=(FunctionalEventAdapter&&) = default;

  void operator()(Event &e) const {
    if (std::ranges::contains(this->event_types, e.type)) {
      this->event_listener(e);
    }
  }

  constexpr bool operator==(const FunctionalEventAdapter &other) const {
    return false;
  }

  constexpr bool operator!=(const FunctionalEventAdapter &other) const {
    return true;
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

template<typename Event>
class EventSource {
  std::vector<event_listener_variant_t<Event>> event_listeners;

protected:
  static void dispatch_event(const std::shared_ptr<EventListener<Event>> &event_listener, Event &e) {
    if constexpr (std::is_same_v<ActionEvent, Event>) {
      event_listener->action_performed(e);
    } else if constexpr (std::is_same_v<KeyEvent, Event>) {
      switch (e.type) {
      case KeyEvent::KEY_PRESSED:
        event_listener->key_pressed(e);
        break;

      case KeyEvent::KEY_TYPED:
        event_listener->key_typed(e);
        break;
      }
    } else if constexpr (std::is_same_v<MouseEvent, Event>) {
      switch (e.type) {
      case MouseEvent::MOUSE_PRESSED:
        event_listener->mouse_pressed(e);
        break;

      case MouseEvent::MOUSE_RELEASED:
        event_listener->mouse_released(e);
        break;
      }
    } else if constexpr (std::is_same_v<MouseClickEvent, Event>) {
      event_listener->mouse_clicked(e);
    } else if constexpr (std::is_same_v<MouseMoveEvent, Event>) {
      event_listener->mouse_moved(e);
    } else if constexpr (std::is_same_v<MouseDragEvent, Event>) {
      event_listener->mouse_dragged(e);
    } else if constexpr (std::is_same_v<MouseWheelEvent, Event>) {
      event_listener->mouse_wheel_moved(e);
    } else if constexpr (std::is_same_v<MouseHoverEvent, Event>) {
      switch (e.type) {
      case MouseHoverEvent::MOUSE_ENTERED:
        event_listener->mouse_entered(e);
        break;

      case MouseHoverEvent::MOUSE_EXITED:
        event_listener->mouse_exited(e);
        break;
      }
    } else if constexpr (std::is_same_v<ItemEvent, Event>) {
      event_listener->item_state_changed(e);
    } else if constexpr (std::is_same_v<FocusEvent, Event>) {
      switch (e.type) {
      case FocusEvent::FOCUS_GAINED:
        event_listener->focus_gained(e);
        break;

      case FocusEvent::FOCUS_LOST:
        event_listener->focus_lost(e);
        break;
      }
    } else if constexpr (std::is_same_v<InvocationEvent, Event>) {
    }
  }

  virtual void process_event(Event &e) {
    for (auto &&event_listener : this->event_listeners) {
      std::visit([&e](auto &&event_listener) {
        using T = std::decay_t<decltype(event_listener)>;
        if constexpr (std::is_same_v<T, std::shared_ptr<EventListener<Event>>>) {
          dispatch_event(event_listener, e);
        } else if constexpr (std::is_same_v<T, BasicFunctionalEventAdapter<Event>>) {
          event_listener(e);
        } else if constexpr (std::is_same_v<T, FunctionalEventAdapter<Event>>) {
          event_listener(e);
        }
      }, event_listener);
    }
  }

protected:
  void add_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto &&l : this->event_listeners) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&l)) {
        if (*event_listener_ptr == event_listener) {
          return;
        }
      }
    }
    this->event_listeners.emplace_back(event_listener);
  }

  template<typename Callable>
  std::enable_if_t<std::is_invocable_v<Callable, Event&>, void> add_event_listener(Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<BasicFunctionalEventAdapter<Event>>(&l)) {
        if (auto target = adapter->event_listener.template target<std::decay_t<Callable>>()) {
          if constexpr (detail::is_global_function_v<Callable, Event>) {
            if (*target == callable) {
              return;
            }
          } else {
            return;
          }
        }
      }
    }
    this->event_listeners.emplace_back(std::in_place_type<BasicFunctionalEventAdapter<Event>>, std::forward<Callable>(callable));
  }

  template<typename Callable, typename E = Event>
  std::enable_if_t<std::is_invocable_v<Callable, E&> and has_type_enum_v<E>, void> add_event_listener(std::vector<typename E::Type> &&event_types, Callable &&callable) {
    for (auto &&l : this->event_listeners) {
      if (auto adapter = std::get_if<FunctionalEventAdapter<E>>(&l)) {
        if (auto target = adapter->event_listener.template target<std::decay_t<Callable>>()) {
          if constexpr (detail::is_global_function_v<Callable, Event>) {
            if (*target == callable) {
              return;
            }
          } else {
            for (auto &&event_type : event_types) {
              if (not std::ranges::contains(adapter->event_types, event_type)) {
                adapter->event_types.emplace_back(event_type);
              }
            }
            return;
          }
        }
      }
    }
    this->event_listeners.emplace_back(std::in_place_type<FunctionalEventAdapter<Event>>, std::move(event_types), std::forward<Callable>(callable));
  }

  void remove_event_listener(const std::shared_ptr<EventListener<Event>> &event_listener) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end(); ++l) {
      if (auto event_listener_ptr = std::get_if<std::shared_ptr<EventListener<Event>>>(&*l)) {
        if (*event_listener_ptr == event_listener) {
          this->event_listeners.erase(l);
          return;
        }
      }
    }
  }

  template<typename Callable>
  void remove_event_listener(const Callable &callable) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<BasicFunctionalEventAdapter<Event>>(&*l)) {
        if (adapter->event_listener.template target<std::decay_t<Callable>>()) {
          l = this->event_listeners.erase(l);
          continue;
        }
      }
      ++l;
    }
  }

  template<typename Callable, typename E = Event>
  std::enable_if_t<has_type_enum_v<E>, void> remove_event_listener(const std::vector<typename E::Type> &types, const Callable &callable) {
    for (auto l = this->event_listeners.begin(); l != this->event_listeners.end();) {
      if (auto adapter = std::get_if<FunctionalEventAdapter<E>>(&*l)) {
        if (adapter->event_listener.template target<std::decay_t<Callable>>()) {
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
struct __event_type_from_callable_or_nothing: std::enable_if<std::is_convertible_v<Callable, FunctionalEventListener<Event>>, Event> {
};

template<typename Callable, typename ... Events>
struct event_type_from_callable_or_nothing: __event_type_from_callable_or_nothing<Callable, Events> ... {
};

template<typename Callable, typename, typename ... Events>
struct event_type_from_callable {
  using type = void;
};

template<typename Callable, typename ... Events>
struct event_type_from_callable<Callable, std::void_t<typename event_type_from_callable_or_nothing<Callable, Events...>::type>, Events...> {
  using type = typename event_type_from_callable_or_nothing<Callable, Events...>::type;
};

template<typename Callable, typename ... Events>
using event_type_from_callable_t = typename event_type_from_callable<Callable, void, Events...>::type;

template<typename Listener, typename = void>
struct event_type_from_listener {
  using type = void;
};

template<typename Listener>
struct event_type_from_listener<Listener, std::void_t<typename Listener::event_type>> {
  using type = typename Listener::event_type;
};

template<typename Listener>
using event_type_from_listener_t = typename event_type_from_listener<Listener>::type;

}

template<typename ... Events>
class BasicEventSource: protected virtual detail::EventSource<Events> ... {
protected:
//
  using detail::EventSource<Events>::process_event...;

public:
  template<typename Listener, typename Event = detail::event_type_from_listener_t<Listener>>
  std::enable_if_t<std::is_convertible_v<Listener*, EventListener<Event>*>, void> add_event_listener(const std::shared_ptr<Listener> &listener) {
    detail::EventSource<Event>::add_event_listener(std::static_pointer_cast<EventListener<Event>>(listener));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<std::is_invocable_v<Callable, Event&>, void> add_event_listener(Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::forward<Callable>(callable));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<std::is_invocable_v<Callable, Event&> and detail::has_type_enum_v<Event>, void> add_event_listener(typename Event::Type type, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { 1, type }, std::forward<Callable>(callable));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<std::is_invocable_v<Callable, Event&> and detail::has_type_enum_v<Event>, void> add_event_listener(std::initializer_list<typename Event::Type> types, Callable &&callable) {
    detail::EventSource<Event>::add_event_listener(std::vector<typename Event::Type> { types.begin(), types.end() }, std::forward<Callable>(callable));
  }

  template<typename Listener, typename Event = detail::event_type_from_listener_t<Listener>>
  std::enable_if_t<std::is_convertible_v<Listener*, EventListener<Event>*>, void> remove_event_listener(const std::shared_ptr<Listener> &listener) {
    detail::EventSource<Event>::remove_event_listener(std::static_pointer_cast<EventListener<Event>>(listener));
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<std::is_invocable_v<Callable, Event&>, void> remove_event_listener(const Callable &callable) {
    detail::EventSource<Event>::remove_event_listener(callable);
  }

  template<typename Callable, typename Event = detail::event_type_from_callable_t<Callable, Events...>>
  std::enable_if_t<std::is_invocable_v<Callable, Event&> and detail::has_type_enum_v<Event>, void> remove_event_listener(typename Event::Type type, const Callable &callable) {
    detail::EventSource<Event>::remove_event_listener(std::vector<typename Event::Type> { 1, type }, callable);
  }
};

}
