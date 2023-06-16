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

int main(int argc, char *argv[]) {
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

  static_assert(std::is_convertible_v<decltype(mouse_listener), FunctionalEventListener<MouseEvent>>);
  static_assert(typeid(mouse_listener) != typeid(mouse_listener2));

  frame->add_event_listener(mouse_listener);
  frame->add_event_listener(MouseEvent::MOUSE_DRAGGED, mouse_listener);
  frame->add_event_listener(MouseEvent::MOUSE_MOVED, mouse_listener);
  frame->add_event_listener(MouseEvent::MOUSE_MOVED, mouse_listener2);

  frame->add_event_listener(MouseEvent::MOUSE_DRAGGED, [](MouseEvent &e) {

  });

  frame->add_event_listener( { MouseEvent::MOUSE_DRAGGED }, [](MouseEvent &e) {

  });

  frame->remove_event_listener(MouseEvent::MOUSE_DRAGGED, mouse_listener);
  frame->remove_event_listener(mouse_listener);

  terminal.run_event_loop();
}
