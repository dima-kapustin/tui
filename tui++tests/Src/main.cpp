#include <tui++/Screen.h>
#include <tui++/terminal/Terminal.h>

using namespace tui;
using namespace tui::terminal;

void test_EnumMask();
void test_utf8();
void test_GlyphIterator();

int main(int argc, char *argv[]) {
  Terminal terminal;
  terminal.set_title("Welcome to tui++");

  //test_EnumMask();
  test_utf8();
  test_GlyphIterator();

  terminal.run_event_loop();
}
