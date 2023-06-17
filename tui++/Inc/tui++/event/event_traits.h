#pragma once

namespace tui {

namespace detail {
template<typename Callable, typename Event>
struct __event_type_from_callable_or_nothing: std::enable_if<std::is_convertible_v<Callable, FunctionalEventListener<Event>>, Event> {
};

template<typename Callable, typename ... Events>
struct event_type_from_callable_or_nothing: __event_type_from_callable_or_nothing<Callable, Events> ... {
};

}

template<typename Callable, typename, typename ... Events>
struct event_type_from_callable {
  using type = void;
};

template<typename Callable, typename ... Events>
struct event_type_from_callable<Callable, std::void_t<typename detail::event_type_from_callable_or_nothing<Callable, Events...>::type>, Events...> {
  using type = typename detail::event_type_from_callable_or_nothing<Callable, Events...>::type;
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
