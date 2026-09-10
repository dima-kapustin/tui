// Exercises the drop shadows (see Shadow): a popup menu shades the cells the
// window beneath it painted, the theme's shadow value is what its window
// casts, one window can be given a shadow of its own, and the one global
// option switches every shadow off.
//
// The text screen emits the painted cell colors as truecolor SGR sequences,
// so the test repaints a single cell at a time and looks for the exact code
// the blend produces -- no stream decoder needed to pick one cell out of a
// whole screen.
#include <tui++/ComboBox.h>
#include <tui++/Frame.h>
#include <tui++/Panel.h>
#include <tui++/PopupMenu.h>
#include <tui++/Screen.h>
#include <tui++/Shadow.h>
#include <tui++/Window.h>

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/event/InvocationEvent.h>
#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/text/TextScreen.h>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace tui;

#define CHECK(cond)                                                                                                    \
  do {                                                                                                                 \
    if (not(cond)) {                                                                                                   \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                            \
      std::abort();                                                                                                    \
    }                                                                                                                  \
  } while (0)

namespace {

// The frame's face and the popup's face: two colors that no other part of the
// scene paints, so the blend of either is unmistakable in the emitted stream.
constexpr auto FRAME_COLOR = Color { 0, 200, 0 };
constexpr auto POPUP_COLOR = Color { 0, 0, 200 };

// The SGR the text screen emits for a cell painted on `color`.
std::string background_code(Color const &color) {
  return "\x1b[48;2;" + std::to_string(int(color.red())) + ';' + std::to_string(int(color.green())) + ';' + std::to_string(int(color.blue())) + 'm';
}

// The color a shadow leaves on a cell painted on `base`: the cell's color
// shifted towards the shadow's by its opacity, the same arithmetic the text
// screen applies to the cell.
Color shaded(Color const &base, Shadow const &shadow) {
  auto blend = [&](uint8_t from, uint8_t to) {
    return uint8_t(std::lround(from * (1 - shadow.opacity) + to * shadow.opacity));
  };
  return Color { blend(base.red(), shadow.color.red()), blend(base.green(), shadow.color.green()), blend(base.blue(), shadow.color.blue()) };
}

// Pops and dispatches the queued repaint invocations, so the frames this test
// shows are actually painted (the tests drive the dispatch by hand).
void drain_events() {
  auto &queue = screen.get_event_queue();
  for (auto i = 0; i < 2000; ++i) {
    auto event = queue.pop(std::chrono::milliseconds(2));
    if (not event) {
      return;
    }
    if (event->id == InvocationEvent::INVOCATION) {
      static_cast<InvocationEvent&>(*event).dispatch();
    }
  }
  assert(!"event queue did not drain");
}

}

void test_Shadow() {
  std::fprintf(stderr, "test_Shadow: popup shadows, their theme values and the global switch\n");

  terminal.set_type("text");

  // The screen emits through std::cout; capture the runs and clear the buffer
  // between the probes, so every assertion looks at one repaint only.
  auto capture = std::ostringstream { };
  auto *old_cout = std::cout.rdbuf(capture.rdbuf());
  screen.repaint_damaged();
  capture.str({ });
  auto take = [&] {
    auto bytes = capture.str();
    capture.str({ });
    return bytes;
  };

  auto dim = screen.get_size();
  CHECK(dim.width > 24 and dim.height > 8);

  // The geometry: the shadow is the window's rectangle, displaced by the
  // offset and grown by the spread on every side.
  auto displaced = (Shadow { BLACK_COLOR, 0.5, { 2, 1 } }).get_area(Rectangle { 10, 4, 20, 6 });
  auto displaced_expected = Rectangle { 12, 5, 20, 6 };
  CHECK(displaced == displaced_expected);
  auto spread = (Shadow { BLACK_COLOR, 0.25, { -1, -1 }, 2 }).get_area(Rectangle { 10, 4, 20, 6 });
  auto spread_expected = Rectangle { 7, 1, 24, 10 };
  CHECK(spread == spread_expected);

  // A frame painted one flat color, with a popup menu opened over it. The
  // popup and its items are painted a second flat color, so a cell tells
  // which of the two windows put it there.
  auto frame = make_component<Frame>();
  frame->set_size(dim);
  frame->set_background_color(FRAME_COLOR);

  auto popup_menu = make_component<PopupMenu>();
  popup_menu->set_background_color(POPUP_COLOR);
  popup_menu->add("Alpha");
  popup_menu->add("Beta");
  for (auto i = 0; i < popup_menu->get_component_count(); ++i) {
    popup_menu->Component::get_component(i)->set_background_color(POPUP_COLOR);
  }

  frame->set_visible(true);
  drain_events();
  (void)take();

  popup_menu->show(frame, 8, 3);
  drain_events();
  (void)take();

  auto popup_window = popup_menu->get_containing_window();
  CHECK(popup_window);
  auto popup = popup_window->get_bounds();

  // The shadow the popup casts is the one the theme defines for a menu popup.
  auto themed = laf::LookAndFeel::get<std::optional<Shadow>>("PopupMenu.Shadow");
  CHECK(themed.has_value());
  CHECK(popup_window->get_shadow() == themed);
  auto shadowed = shaded(FRAME_COLOR, *themed);

  // Repaints one cell and hands back what the screen emitted for it: a full
  // repaint emits every cell of the screen, while the interesting question is
  // what one particular cell -- inside the popup, on its shadow, on the frame
  // -- is painted on.
  auto cell = [&](int x, int y) {
    dynamic_cast<TextScreen&>(screen).clear();
    capture.str({ });
    screen.add_damage(Rectangle { x, y, 1, 1 });
    screen.repaint_damaged();
    return take();
  };

  // The frame's own cells are painted as they are...
  auto painted = cell(popup.x - 4, popup.y + 1);
  CHECK(painted.find(background_code(FRAME_COLOR)) != std::string::npos);

  // ... and the cells along the popup's right and bottom edge carry the
  // frame's color dimmed towards the shadow's: the theme's offset (2 cells
  // right, 1 down) covers two columns beside the window and, in the row below
  // it, the window's width to the right of where the window's own left edge
  // was shifted to.
  auto right = cell(popup.right(), popup.y + 1);
  CHECK(right.find(background_code(shadowed)) != std::string::npos);
  auto below = cell(popup.x + themed->offset.x, popup.bottom());
  CHECK(below.find(background_code(shadowed)) != std::string::npos);

  // The cells under the popup itself keep the popup's own colors: the shadow
  // runs beneath the window, never over its face.
  auto inside = cell(popup.x + 1, popup.y + 1);
  CHECK(inside.find(background_code(POPUP_COLOR)) != std::string::npos);
  CHECK(inside.find(background_code(shaded(POPUP_COLOR, *themed))) == std::string::npos);

  // The one global option switches every shadow off, whatever the theme
  // defines...
  Shadow::set_enabled(false);
  CHECK(not Shadow::is_enabled());
  (void)take(); // the switch repaints the screen
  auto unshaded = cell(popup.right(), popup.y + 1);
  CHECK(unshaded.find(background_code(FRAME_COLOR)) != std::string::npos);
  CHECK(unshaded.find(background_code(shadowed)) == std::string::npos);

  // ... and on again.
  Shadow::set_enabled(true);
  (void)take();
  CHECK(cell(popup.right(), popup.y + 1).find(background_code(shadowed)) != std::string::npos);

  // A cell the program never painted shows the terminal's own default color,
  // and a shadow can only shade it when the terminal said what that color is
  // (the screen asks with OSC 10/11 when its event loop starts). Without an
  // answer the cell is left as it is; with one, the shadow blends against it
  // and paints the result explicitly.
  {
    auto text_screen = dynamic_cast<TextScreen*>(&screen);
    CHECK(text_screen);

    // The reports the query is answered with parse into the colors they name:
    // "rgb:ffff/ffff/ffff" (xterm sends four hex digits per component), the
    // shorter forms, and #RRGGBB. Anything else has no color.
    auto white = std::optional<Color> { WHITE_COLOR };
    auto black = std::optional<Color> { BLACK_COLOR };
    auto orange = std::optional<Color> { Color { 255, 128, 0 } };
    CHECK(Terminal::parse_color_spec("rgb:ffff/ffff/ffff") == white);
    CHECK(Terminal::parse_color_spec("rgb:00/00/00") == black);
    CHECK(Terminal::parse_color_spec("rgb:ff/80/00") == orange);
    CHECK(Terminal::parse_color_spec("#ff8000") == orange);
    CHECK(not Terminal::parse_color_spec("rgb:ff/80").has_value());
    CHECK(not Terminal::parse_color_spec("green").has_value());

    // A cell nobody painted: on_window_removed resets a cell to the empty
    // state (what a closed window leaves behind, and what the screen's back
    // buffer starts from), and the graphics the screen paints its windows
    // with then blends the shadow into it -- the same primitive a window's
    // shadow is painted with.
    auto nobody = Rectangle { popup.right(), popup.y + 1, 1, 1 };
    auto blend_cell = [&] {
      text_screen->on_window_removed(nobody);
      auto graphics = screen.get_graphics();
      graphics->blend_rect(nobody, themed->color, themed->opacity);
      capture.str({ });
      text_screen->flush();
      return take();
    };

    // Without the terminal's own colors there is nothing to blend against,
    // and the cell keeps the terminal's default: no explicit background is
    // emitted for it at all.
    text_screen->set_default_colors(std::nullopt, std::nullopt);
    CHECK(blend_cell().find("\x1b[48") == std::string::npos);

    // With them, the shadow darkens the terminal's own background and paints
    // the result explicitly.
    auto terminal_background = Color { 255, 255, 255 };
    text_screen->set_default_colors(WHITE_COLOR, terminal_background);
    CHECK(blend_cell().find(background_code(shaded(terminal_background, *themed))) != std::string::npos);

    text_screen->set_default_colors(std::nullopt, std::nullopt);
  }

  // One window can be given a shadow of its own (any color, offset and
  // opacity -- a lighter color lightens instead of darkening), and can be
  // left without one: the theme's value is only the default.
  auto light = Shadow { WHITE_COLOR, 0.5, { 1, 0 } };
  popup_window->set_shadow(light);
  CHECK(cell(popup.right(), popup.y + 1).find(background_code(shaded(FRAME_COLOR, light))) != std::string::npos);
  popup_window->set_shadow(std::nullopt);
  CHECK(not popup_window->get_shadow().has_value());
  (void)take();
  CHECK(cell(popup.right(), popup.y + 1).find(background_code(FRAME_COLOR)) != std::string::npos);

  // Closing the popup takes its shadow with it: the cells it shaded are the
  // frame's again.
  popup_menu->set_visible(false);
  drain_events();
  (void)take();
  auto restored = cell(popup.right(), popup.y + 1);
  CHECK(restored.find(background_code(FRAME_COLOR)) != std::string::npos);
  CHECK(restored.find(background_code(shadowed)) == std::string::npos);

  // A combo box dropdown is a popup menu of its own (Swing's
  // BasicComboBoxUI popup) and shades like the combo box, not like a menu.
  auto content = make_component<Panel>();
  frame->add(content);
  auto combo = make_component<ComboBox>(std::vector<std::string> { "Alpha", "Beta", "Gamma" });
  combo->set_name("shadow combo");
  content->add(combo);
  frame->validate();
  drain_events();
  (void)take();

  combo->set_popup_visible(true);
  drain_events();
  (void)take();

  // The dropdown's window sits right under the combo (the combo itself lives
  // in the frame), the way the combo dropdown tests find it.
  auto combo_at = combo->get_location_on_screen();
  auto combo_window = screen.get_window_at(combo_at.x + 2, combo_at.y + combo->get_height() + 1);
  CHECK(combo_window);
  CHECK(combo_window.get() != frame.get());
  auto combo_shadow = laf::LookAndFeel::get<std::optional<Shadow>>("ComboBox.Shadow");
  CHECK(combo_shadow.has_value());
  CHECK(combo_window->get_shadow() == combo_shadow);

  combo->set_popup_visible(false);
  drain_events();
  (void)take();

  // Leave the screen as the test found it: no window and the default switch.
  frame->set_visible(false);
  drain_events();
  (void)take();
  CHECK(Shadow::is_enabled());

  std::cout.rdbuf(old_cout);
}
