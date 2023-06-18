#include <tui++/event/EventSource.h>
#include <tui++/event/MouseEvent.h>

#include <memory>
#include <cassert>

using namespace tui;

static_assert(detail::is_one_of_v<MouseEvent, KeyEvent, ItemEvent, MouseEvent>);

class EventSource_A: public EventSource<KeyEvent, ItemEvent, MouseEvent> {
};

class EventSource_B: public EventSourceExtension<EventSource_A, WindowEvent> {
public:
  size_t get_mouse_event_listener_count() const {
    return detail::SingleEventSource<MouseEvent>::event_listeners.size();
  }

  EventTypeMask get_event_mask() const {
    return this->event_mask;
  }
};

void g(MouseEvent &e) {

}

void f(MouseEvent &e) {

}

static_assert(typeid(f) == typeid(g));

struct X {
  void g(MouseEvent &e) {

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

class ML: public EventListener<MouseEvent> {
public:
  virtual void mouse_pressed(MouseEvent &e) override {
  }

  virtual void mouse_released(MouseEvent &e) override {

  }
};

void test_EventSource() {
  std::function fg = g;
  assert(*fg.target<void (*)(MouseEvent&)>() == g);

  auto mouse_listener = [&fg](MouseEvent &e) {
  };

  static_assert(std::is_convertible_v<decltype(mouse_listener), FunctionalEventListener<MouseEvent>>);

  auto mouse_listener2 = [&fg](MouseEvent &e) {
  };

// std::function uses typeid() in template<F> std::function::target();
  static_assert(typeid(mouse_listener) != typeid(mouse_listener2));

  auto event_source_b = std::make_unique<EventSource_B>();
  auto ml = std::make_shared<ML>();

  event_source_b->add_event_listener(ml);
  assert(event_source_b->get_mouse_event_listener_count() == 1);

  event_source_b->add_event_listener(mouse_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 2);

  event_source_b->add_event_listener(std::static_pointer_cast<EventListener<MouseEvent>>(ml));
  assert(event_source_b->get_mouse_event_listener_count() == 2);

  event_source_b->add_event_listener(g);
  assert(event_source_b->get_mouse_event_listener_count() == 3);

  event_source_b->add_event_listener(f);
  assert(event_source_b->get_mouse_event_listener_count() == 4);

  event_source_b->add_event_listener(f);
  assert(event_source_b->get_mouse_event_listener_count() == 4);

  event_source_b->add_event_listener(MouseEvent::MOUSE_PRESSED, mouse_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 5);

  event_source_b->add_event_listener(MouseEvent::MOUSE_RELEASED, mouse_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 5);

  event_source_b->add_event_listener(MouseEvent::MOUSE_RELEASED, mouse_listener2);
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  event_source_b->add_event_listener(MouseEvent::MOUSE_PRESSED, [](MouseEvent &e) {
  });
  assert(event_source_b->get_mouse_event_listener_count() == 7);

  event_source_b->add_event_listener( { MouseEvent::MOUSE_PRESSED }, [](MouseEvent &e) {
  });
  assert(event_source_b->get_mouse_event_listener_count() == 8);

  event_source_b->remove_event_listener(MouseEvent::MOUSE_PRESSED, mouse_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 8);

  event_source_b->remove_event_listener(mouse_listener);
  assert(event_source_b->get_mouse_event_listener_count() == 7);

  event_source_b->remove_event_listener(std::static_pointer_cast<EventListener<MouseEvent>>(ml));
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  event_source_b->remove_event_listener(ml);
  assert(event_source_b->get_mouse_event_listener_count() == 6);

  assert(event_source_b->get_event_mask() == EventType::MOUSE);

  auto window_listener = [](WindowEvent &e) {
  };

  event_source_b->add_event_listener(WindowEvent::WINDOW_ACTIVATED, window_listener);
  assert(event_source_b->get_event_mask() == (EventType::MOUSE|EventType::WINDOW));

  event_source_b->remove_event_listener(WindowEvent::WINDOW_ACTIVATED, window_listener);
  assert(event_source_b->get_event_mask() == EventType::MOUSE);
}
