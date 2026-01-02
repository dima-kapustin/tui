#include <tui++/event/EventSource.h>
#include <tui++/event/MouseEvent.h>

#include <memory>
#include <cassert>

using namespace tui;
using namespace tui::detail;

static_assert(detail::is_one_of_v<MousePressEvent, KeyEvent, ItemEvent, MousePressEvent>);

class EventSource_A: public EventSource<KeyEvent, ItemEvent, MousePressEvent> {
};

class EventSource_B: public EventSourceExtension<EventSource_A, WindowEvent> {
  using base = EventSourceExtension<EventSource_A, WindowEvent>;
public:
  size_t get_mouse_event_listener_count() const {
    return base::get_event_listener_count<MousePressEvent>();
  }
};

void g(MousePressEvent &e) {

}

void f(MousePressEvent &e) {

}

static_assert(typeid(f) == typeid(g));

struct X {
  void g(MousePressEvent &e) {

  }
};

struct A {
  static void f() {

  }
};

struct B {
  static void f() {

  }
};

static_assert(typeid(A::f) == typeid(B::f));

class ML: public EventListener<MousePressEvent> {
public:
  virtual void mouse_pressed(MousePressEvent &e) override {
  }

  virtual void mouse_released(MousePressEvent &e) override {

  }
};

void test_EventSource() {
  std::function fg = g;
  assert(*fg.target<void (*)(MousePressEvent&)>() == g);

  auto mouse_press_listener = [&fg](MousePressEvent &e) {
  };

  static_assert(is_callable_listener_v<decltype(mouse_press_listener), MousePressEvent>);

  static_assert(std::is_convertible_v<decltype(mouse_press_listener), FunctionalEventListener<MousePressEvent>>);

  auto mouse_press_listener_2 = [&fg](MousePressEvent &e) {
  };

// std::function uses typeid() in template<F> std::function::target();
  static_assert(typeid(mouse_press_listener) != typeid(mouse_press_listener_2));

  auto event_source_b = std::make_unique<EventSource_B>();
  auto ml = std::make_shared<ML>();

  event_source_b->add_listener(ml);
  assert(event_source_b->get_mouse_event_listener_count() == 1);

  event_source_b->add_listener(mouse_press_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 2);

  event_source_b->add_listener(std::static_pointer_cast<EventListener<MousePressEvent>>(ml));
  assert(event_source_b->get_mouse_event_listener_count() == 2);

  static_assert(is_function_pointer_listener_v<decltype(g), MousePressEvent>);
  static_assert(is_callable_listener_v<decltype(g), MousePressEvent>);

  event_source_b->add_listener(g);
  assert(event_source_b->get_mouse_event_listener_count() == 3);

  event_source_b->add_listener(f);
  assert(event_source_b->get_mouse_event_listener_count() == 4);

  event_source_b->add_listener(f);
  assert(event_source_b->get_mouse_event_listener_count() == 4);

  event_source_b->add_listener(MousePressEvent::MOUSE_PRESSED, mouse_press_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 5);

  event_source_b->add_listener(MousePressEvent::MOUSE_RELEASED, mouse_press_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 5);

  event_source_b->add_listener(MousePressEvent::MOUSE_RELEASED, mouse_press_listener_2);
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  event_source_b->add_listener(MousePressEvent::MOUSE_PRESSED, [](MousePressEvent &e) {
  });
  assert(event_source_b->get_mouse_event_listener_count() == 7);

  event_source_b->add_listener( { MousePressEvent::MOUSE_PRESSED }, [](MousePressEvent &e) {
  });
  assert(event_source_b->get_mouse_event_listener_count() == 8);

  event_source_b->remove_listener(MousePressEvent::MOUSE_PRESSED, mouse_press_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 8);

  event_source_b->remove_listener(mouse_press_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 7);

  event_source_b->remove_listener(std::static_pointer_cast<EventListener<MousePressEvent>>(ml));
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  event_source_b->remove_listener(ml);
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  assert(event_source_b->get_event_listener_mask() == EventType::MOUSE_PRESS);

  auto window_listener = [](WindowEvent &e) {
  };

  event_source_b->add_listener(WindowEvent::WINDOW_ACTIVATED, window_listener);
  assert(event_source_b->get_event_listener_mask() == (EventType::MOUSE_PRESS | EventType::WINDOW));
  assert(event_source_b->has_event_listeners(EventType::MOUSE_PRESS | EventType::WINDOW));

  event_source_b->remove_listener(WindowEvent::WINDOW_ACTIVATED, window_listener);
  assert(event_source_b->get_event_listener_mask() == EventType::MOUSE_PRESS);
  assert(event_source_b->has_event_listeners(EventType::MOUSE_PRESS));
  assert(not event_source_b->has_event_listeners(EventType::WINDOW));
}
