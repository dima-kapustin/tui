// Crash diagnostics probe: exercises the three fatal paths of the
// diagnostics module (tui++/util/diagnostics.h) through the REAL terminal
// wiring and checks that each one logs its report before the process dies.
//
//   tools/crash_probe.exe dispatch   - an exception escapes an event dispatch
//   tools/crash_probe.exe signal     - raise(SIGSEGV) (the CRT signal path)
//   tools/crash_probe.exe segv       - a real access violation (the SEH path)
//
// Every mode ends the process abnormally on purpose; run it with stderr
// redirected and grep for the "[tui++ fatal]" markers:
//
//   dispatch: "uncaught exception while dispatching:" + "what(): probe boom"
//   signal:   "signal SIGSEGV (11)"
//   segv:     "unhandled exception 0x...c0000005"
//
// Build (from the repo root):
//   g++ -g -std=gnu++26 -DUNICODE -Itui++/Inc tools/crash_probe.cpp \
//       build/tui++/libtui++.a -lkernel32 -luser32 -lgdi32 -lwinspool \
//       -lshell32 -lole32 -loleaut32 -luuid -lcomdlg32 -ladvapi32 \
//       -o tools/crash_probe.exe

#include <tui++/Screen.h>
#include <tui++/Frame.h>
#include <tui++/Graphics.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/util/diagnostics.h>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

using namespace tui;

namespace {

// A minimal Screen exposing dispatch_event (the real loops call it through
// TextScreen, which keeps it protected): lets the probe drive one event
// through the exact dispatch wrapper the loops use.
struct ProbeScreen: Screen {
  void run_event_loop() override {
  }
  std::unique_ptr<Graphics> get_graphics() override {
    return {};
  }
  std::unique_ptr<Graphics> get_graphics(Rectangle const &) override {
    return {};
  }
  std::shared_ptr<laf::LookAndFeel> get_look_and_feel() const override {
    return {};
  }
  std::shared_ptr<TextMetrics> get_text_metrics() const override {
    return {};
  }
  void refresh() override {
  }

  // Screen::dispatch_event is protected (only the event loops call it); this
  // lets the probe drive one event through the exact wrapper the loops use.
  void drive(std::shared_ptr<Event> const &event) {
    dispatch_event(*event);
  }
};

}

int main(int argc, char **argv) {
  auto mode = argc > 1 ? argv[1] : "dispatch";

  if (std::strcmp(mode, "signal") == 0) {
    // SIGSEGV through the CRT: the terminal's signal handler logs it. The
    // global terminal (constructed before main) installed the handlers.
    std::raise(SIGSEGV);
    return 0; // not reached
  }

  if (std::strcmp(mode, "segv") == 0) {
    // A genuine access violation: the Windows unhandled-exception filter.
    auto boom = reinterpret_cast<volatile int *>(std::uintptr_t(0));
    *boom = 42;
    return 0; // not reached
  }

  // An exception thrown by a window listener during dispatch, driven through
  // the same wrapper the event loops use.
  auto probe_screen = ProbeScreen { };
  auto frame = make_component<Frame>();
  frame->set_name("crash probe frame");
  frame->add_listener([](KeyEvent &) {
    throw std::runtime_error("probe boom");
  });

  probe_screen.post<KeyEvent>(frame, KeyEvent::KEY_PRESSED, KeyEvent::VK_F9, InputEvent::NO_MODIFIERS);
  auto event = probe_screen.get_event_queue().pop();
  probe_screen.drive(event); // logs the context, then rethrows

  std::fprintf(stderr, "probe: dispatch returned (unexpected)\n");
  return 0;
}
