#include <tui++/Shadow.h>

#include <tui++/Screen.h>

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {

// The global switch is a theme property like any other, so a look-and-feel
// can decide it and a program can read it back; the helpers here just save
// every caller from knowing the key.
constexpr auto SHADOW_ENABLED_KEY = "Shadow.Enabled";

bool Shadow::is_enabled() {
  return laf::LookAndFeel::get<bool>(SHADOW_ENABLED_KEY, true);
}

void Shadow::set_enabled(bool value) {
  if (is_enabled() == value) {
    return;
  }

  laf::LookAndFeel::put(SHADOW_ENABLED_KEY, value);

  // A shadow is painted outside its window, into cells the window's own
  // repaint never reaches, so switching shadows on or off needs a full
  // repaint: the windows below paint over the shadow that is gone, and each
  // open popup shades its rim again. A screen with nothing on it has nothing
  // to repaint, and repainting it would only write the terminal blank.
  if (screen.has_windows()) {
    screen.refresh();
  }
}

}
