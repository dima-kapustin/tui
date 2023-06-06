#include <tui++/terminal/Terminal.h>

using namespace tui;
using namespace tui::terminal;

void test_EnumMask();
void test_utf8();
void test_GlyphIterator();

int main(int argc, char *argv[]) {
  test_EnumMask();
  test_utf8();
  test_GlyphIterator();

  Terminal terminal;
  terminal.set_title("Welcome to tui++");

  auto g = terminal.get_graphics();

  terminal.run_event_loop();
}
