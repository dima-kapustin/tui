#pragma once

#include <tui++/Color.h>
#include <tui++/Insets.h>
#include <tui++/Point.h>
#include <tui++/Rectangle.h>
#include <tui++/Themable.h>

#include <algorithm>

namespace tui {

class Graphics;

/**
 * The drop shadow a floating window casts over the windows beneath it: a
 * popup menu, a combo box dropdown or a dialog sits above the rest of the
 * screen, and the area it shades reads darker -- or lighter, with a light
 * color -- than the rest.
 *
 * The shadow is not an opaque slab: the cells (pixels) it covers are shifted
 * towards `color` by `opacity`, so the content underneath stays visible, only
 * dimmed (Swing's AlphaComposite.SRC_OVER over the existing content). The
 * window's own rectangle is left alone: a shadow paints only the rim the
 * window does not cover, which `offset` (the shadow's displacement, e.g. 2
 * columns and 1 row) and `spread` (an extra margin on every side) shape. In
 * cells (the text screen) the offset and the spread count cells; on the pixel
 * (sixel) screen they count pixels.
 *
 * A shadow is a theme value, so it is configured the way everything else is:
 *
 *   LookAndFeel::put("PopupMenu.Shadow", Shadow { BLACK_COLOR, 0.5, { 2, 1 } });
 *
 * (the window's kind selects the key, see Window::get_shadow_key), and every
 * shadow switches off with the one global option
 *
 *   Shadow::set_enabled(false);
 */
struct Shadow: public Themable {
  Color color { BLACK_COLOR };

  // How much of the shadow color the shaded cells take: 0 leaves them as they
  // are, 1 paints the shadow color solid (the content underneath is then
  // hidden, not dimmed).
  double opacity = 0.5;

  // The shadow's displacement from the window's bounds; down-right by
  // default, like a light source above and to the left.
  Point offset { 2, 1 };

  // An extra margin around the displaced bounds, on every side.
  int spread = 0;

  constexpr Shadow() = default;

  constexpr Shadow(Color const &color, double opacity = 0.5, Point const &offset = { 2, 1 }, int spread = 0) :
      color(color), opacity(opacity), offset(offset), spread(spread) {
  }

  constexpr bool operator==(Shadow const &other) const {
    return this->color == other.color//
        and this->opacity == other.opacity//
        and this->offset == other.offset//
        and this->spread == other.spread;
  }

  // The rectangle this shadow shades for a window occupying `bounds`.
  constexpr Rectangle get_area(Rectangle const &bounds) const {
    return { // the displaced bounds, grown by the spread on every side
      bounds.x + offset.x - spread, //
      bounds.y + offset.y - spread, //
      bounds.width + 2 * spread, //
      bounds.height + 2 * spread };
  }

  // The room the shadow needs beyond `bounds` -- the part of its area that
  // falls outside the rectangle it is cast by, as insets. A border reserves it
  // so the layout leaves the shadow its room (see ButtonBorder).
  constexpr Insets get_insets() const {
    return { // top, left, bottom, right
      std::max(0, spread - offset.y), //
      std::max(0, spread - offset.x), //
      std::max(0, offset.y + spread), //
      std::max(0, offset.x + spread) };
  }

  // Shades the cells the shadow covers outside `bounds` (the rim above, right,
  // below and left of it) with the shadow's color at its opacity. Only the rim
  // is painted: the rectangle the shadow is cast by covers its own middle, and
  // whatever it leaves unpainted there (a translucent window) must not be
  // tinted by the shadow running underneath it.
  void paint(Graphics &g, Rectangle const &bounds) const;

  // The global shadow switch (the theme's "Shadow.Enabled" property, true by
  // default): while it is off no window paints a shadow, whatever the theme
  // defines for its kind.
  static bool is_enabled();

  // Flips the global switch and repaints the screen, so the shadows appear
  // or disappear right away (a screen with nothing on it has nothing to
  // repaint, and is left alone).
  static void set_enabled(bool value);
};

}
