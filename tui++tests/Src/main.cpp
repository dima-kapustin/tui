#include <tui++/Screen.h>
#include <tui++/Terminal.h>

using namespace tui;

int main(int argc, char* argv[]) {
  Terminal::set_title("Wlecome to tui++");
  Screen::run_event_loop();
}
