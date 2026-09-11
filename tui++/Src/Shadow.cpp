#include <tui++/Shadow.h>

#include <tui++/Graphics.h>
#include <tui++/Screen.h>

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {

void Shadow::paint(Graphics &g, Rectangle const &bounds) const {
  auto area = get_area(bounds) & g.get_clip_rect();
  if (area.empty()) {
    return;
  }

  auto paint = [&](Rectangle const &part) {
    if (not part.empty()) {
      g.blend_rect(part, this->color, this->opacity);
    }
  };

  // Only the parts of the shadow the rectangle does not cover are visible, so
  // paint the area as the bands above, right, below and left of it (in
  // whatever order: they do not overlap).
  auto below_top = std::max(area.y, bounds.bottom());
  paint({ area.x, area.y, area.width, std::min(area.bottom(), bounds.y) - area.y });

  auto middle_top = std::max(area.y, bounds.y);
  auto middle_bottom = std::min(area.bottom(), bounds.bottom());
  if (middle_bottom > middle_top) {
    paint({ area.x, middle_top, bounds.x - area.x, middle_bottom - middle_top });
    paint({ bounds.right(), middle_top, area.right() - bounds.right(), middle_bottom - middle_top });
  }

  paint({ area.x, below_top, area.width, area.bottom() - below_top });
}

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
  // open popup shades its rim again. The switch also changes the room a
  // button's shadow takes (see ButtonBorder), so the trees are revalidated
  // before the repaint that shows the new layout. A screen with nothing on it
  // has nothing to repaint, and repainting it would only write the terminal
  // blank.
  if (screen.has_windows()) {
    screen.revalidate_windows();
    screen.refresh();
  }
}

}
