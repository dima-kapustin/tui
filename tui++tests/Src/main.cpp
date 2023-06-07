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
  terminal.flush();

  terminal.post([&terminal] {
    auto g = terminal.get_graphics();
    g->set_foreground_color(GREEN_COLOR);
    g->draw_string("Привет, мир!", 1, 1, Attribute::STANDOUT);
    g->set_stroke(Stroke::DOUBLE);
    g->draw_rect(0, 0, 14, 3);
    g->flush();
  });

  terminal.run_event_loop();
}
