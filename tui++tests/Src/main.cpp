#include <tui++/Event.h>
#include <tui++/Frame.h>
#include <tui++/MenuBar.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalGraphics.h>

#include <tui++/MenuSelectionManager.h>

#include <iostream>

using namespace tui;

void test_utf8();
void test_Char();
void test_EnumMask();
void test_KeyStroke();
void test_EventSource();
void test_CharIterator();

int main(int argc, char *argv[]) {
  test_utf8();
  test_Char();
  test_EnumMask();
  test_KeyStroke();
  test_EventSource();
  test_CharIterator();

  this_terminal.set_title("Welcome to tui++");
  this_terminal.flush();

//  this_terminal.post([&terminal] {
//    auto g = terminal.get_graphics();
//    g->set_foreground_color(GREEN_COLOR);
//    g->draw_string("Привет, мир!", 1, 7, Attribute::STANDOUT);
//    g->set_stroke(Stroke::DOUBLE);
//    auto size = terminal.get_size();
//    g->draw_rounded_rect(0, 0, size.width, size.height);
//    g->flush();
//  });

  auto menu_bar = make_component<MenuBar>();

  auto frame = make_component<Frame>();
  frame->set_background_color(GREEN_COLOR);
  frame->set_menu_bar(menu_bar);
//  frame->add_property_change_listener("visible", [](PropertyChangeEvent &e) {
//    std::cout << e.property_name << std::endl;
//  });
  frame->set_size(this_terminal.get_size());
  frame->pack();
  frame->set_visible(true);

  frame->add_listener([](MouseMoveEvent &e) {
//    std::cout << e << std::endl;
  });

  this_terminal.run_event_loop();
}
