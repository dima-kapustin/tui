#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <tui++/Frame.h>
#include <tui++/event/MouseEvent.h>

using namespace tui;
using namespace tui::terminal;

void test_utf8();
void test_Char();
void test_EnumMask();
void test_KeyStroke();
void test_CharIterator();

void g(MouseEvent &e) {

}

void f(MouseEvent &e) {

}

std::function gg = g;

struct X {
  void g(MouseEvent &e) {

  }
};

template<typename F, typename Event>
struct is_global_function: std::is_convertible<F, void (*)(Event&)> {
};

static_assert(is_global_function<decltype(g), MouseEvent>::value);
//static_assert(is_global_function<decltype(gg), MouseEvent>::value);

//static_assert(detail::is_global_function_v<decltype(g), MouseEvent>);

struct A {
  static void f() {

  }
};

struct B {
  static void f() {

  }
};

class ML: public EventListener<MouseEvent> {
public:
  virtual void mouse_pressed(MouseEvent &e) override {
  }

  virtual void mouse_released(MouseEvent &e) override {

  }
};

int main(int argc, char *argv[]) {

  assert(*gg.target<void (*)(MouseEvent&)>() == g);

  test_utf8();
  test_Char();
  test_EnumMask();
  test_KeyStroke();
  test_CharIterator();

  Terminal terminal;
  terminal.set_title("Welcome to tui++");
  terminal.flush();

  terminal.post([&terminal] {
    auto g = terminal.get_graphics();
    g->set_foreground_color(GREEN_COLOR);
    g->draw_string("Привет, мир!", 1, 1, Attribute::STANDOUT);
    g->set_stroke(Stroke::DOUBLE);
    g->draw_rounded_rect(0, 0, 14, 3);
    g->flush();
  });

  auto frame = terminal.create_window<Frame>();

  auto mouse_listener = [&terminal](MouseEvent &e) {

  };

  auto mouse_listener2 = [&terminal](MouseEvent &e) {

  };

  auto ml = std::make_shared<ML>();

  static_assert(std::is_convertible_v<decltype(mouse_listener), FunctionalEventListener<MouseEvent>>);
  static_assert(typeid(mouse_listener) != typeid(mouse_listener2));
//  static_assert(typeid(A::f) != typeid(B::f));
//  static_assert(typeid(f) != typeid(g));

  frame->add_event_listener(mouse_listener);
//  frame->add_event_listener(ml);
  frame->add_event_listener(std::static_pointer_cast<EventListener<MouseEvent>>(ml));

  frame->add_event_listener(g);
  frame->add_event_listener(f);
  frame->add_event_listener(f);

  frame->add_event_listener(MouseEvent::MOUSE_PRESSED, mouse_listener);
  frame->add_event_listener(MouseEvent::MOUSE_RELEASED, mouse_listener);
  frame->add_event_listener(MouseEvent::MOUSE_RELEASED, mouse_listener2);

  frame->add_event_listener(MouseEvent::MOUSE_PRESSED, [](MouseEvent &e) {

  });

  frame->add_event_listener( { MouseEvent::MOUSE_PRESSED }, [](MouseEvent &e) {

  });

  frame->remove_event_listener(MouseEvent::MOUSE_PRESSED, mouse_listener);
  frame->remove_event_listener(mouse_listener);
  frame->remove_event_listener(std::static_pointer_cast<EventListener<MouseEvent>>(ml));

  terminal.run_event_loop();
}
