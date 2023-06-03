#include <tui++/Screen.h>
#include <tui++/Terminal.h>

using namespace tui;

void test_EnumMask();

int main(int argc, char* argv[]) {
  Terminal::set_title("Wlecome to tui++");

  test_EnumMask();

  Screen::run_event_loop();
}
