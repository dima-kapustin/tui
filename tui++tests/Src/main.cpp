#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/TerminalGraphics.h>

using namespace tui;
using namespace tui::terminal;

void test_utf8();
void test_EnumMask();
void test_GlyphIterator();

int main(int argc, char *argv[]) {
  test_utf8();
  test_EnumMask();
  test_GlyphIterator();

  Terminal terminal;
  terminal.set_title("Welcome to tui++");

  terminal.post([&terminal] {
    auto g = terminal.get_graphics();
    g->set_foreground_color(GREEN_COLOR);
    g->draw_string("Привет, мир!", 1, 1, Attribute::STANDOUT);
    g->flush();
  });

  terminal.run_event_loop();
}
