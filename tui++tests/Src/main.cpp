#include <tui++/Event.h>
#include <tui++/Frame.h>
#include <tui++/Panel.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/LineBorder.h>

#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>

#include <tui++/terminal/Terminal.h>

#include <iostream>
#include <string_view>

using namespace tui;

void test_utf8();
void test_Char();
void test_EnumMask();
void test_KeyStroke();
void test_EventSource();
void test_CharIterator();
void test_Action();
void test_Color();

auto make_file_menu() {
  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');

  auto file_chooser_item = make_component<MenuItem> ("JFileChooser", 'F');
  file_chooser_item->add_listener([](ActionEvent &e) {

  });
  file_menu->add(file_chooser_item);

  file_menu->add_separator();

  auto exit_item = make_component<MenuItem>("Exit", 'x');
  exit_item->add_listener([](ActionEvent &e) {

  });
  file_menu->add(exit_item);
  return file_menu;
}

auto make_edit_menu() {
  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->set_mnemonic('E');

  auto cut_item = make_component<MenuItem> ("Cut");
  cut_item->add_listener([](ActionEvent &e) {

  });
  edit_menu->add(cut_item);

  auto copy_item = make_component<MenuItem> ("Copy");
  copy_item->add_listener([](ActionEvent &e) {

  });
  edit_menu->add(copy_item);

  auto paste_item = make_component<MenuItem> ("Paste");
  paste_item->add_listener([](ActionEvent &e) {

  });
  edit_menu->add(paste_item);

  edit_menu->add_separator();

  auto delete_item = make_component<MenuItem>("Delete");
  delete_item->add_listener([](ActionEvent &e) {

  });
  edit_menu->add(delete_item);
  return edit_menu;
}

int main(int argc, char *argv[]) {
  test_utf8();
  test_Char();
  test_EnumMask();
  test_KeyStroke();
  test_EventSource();
  test_CharIterator();
  test_Action();
  test_Color();

  terminal.set_title("Welcome to tui++");

  // Select the rendering backend: "text" for the escape-sequence terminal
  // (the default) or "graphic" for the pixel-level sixel terminal.
  terminal.set_type(argc > 1 and std::string_view(argv[1]) == "sixel" ? "graphic" : "text");

//  terminal.post([&terminal] {
//    auto g = terminal.get_graphics();
//    g->set_foreground_color(GREEN_COLOR);
//    g->draw_string("Привет, мир!", 1, 7, Attribute::STANDOUT);
//    g->set_stroke(Stroke::DOUBLE);
//    auto size = terminal.get_size();
//    g->draw_rounded_rect(0, 0, size.width, size.height);
//    g->flush();
//  });

  auto menu_bar = make_component<MenuBar>();
  menu_bar->add(make_file_menu());
  menu_bar->add(make_edit_menu());

  auto frame = make_component<Frame>();
  frame->set_background_color(GREEN_COLOR);
  frame->set_menu_bar(menu_bar);
//  frame->add_property_change_listener("visible", [](PropertyChangeEvent &e) {
//    std::cout << e.property_name << std::endl;
//  });
  // Size the frame to the screen: cell units on the text screen, pixels on
  // the graphic (sixel) screen, where components address individual pixels.
  frame->set_size(screen.get_size());

  // Inset the content so the frame's background is visible around the panel.
  frame->get_content_pane()->set_border(std::make_shared<EmptyBorder>(2, 2, 2, 2));

  auto panel = make_component<Panel>();
  panel->set_background_color(BLUE_COLOR);
  panel->set_border(std::make_shared<LineBorder>(Stroke::HEAVY, YELLOW_COLOR));
  frame->add(panel);

  frame->set_visible(true);

  frame->add_listener([](MouseMoveEvent &e) {
//    std::cout << e << std::endl;
  });

  terminal.run_event_loop();
}
