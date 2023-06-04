#include <tui++/Screen.h>
#include <tui++/terminal/Terminal.h>

using namespace tui;
using namespace tui::terminal;

void test_EnumMask();

int main(int argc, char *argv[]) {
  Terminal terminal;
  terminal.set_title("Welcome to tui++");

  //test_EnumMask();

  terminal.run_event_loop();
}
