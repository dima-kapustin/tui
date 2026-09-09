#include <tui++/lookandfeel/ButtonUI.h>

#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/lookandfeel/basic/ToggleIndicator.h>

#include <tui++/AbstractButton.h>
#include <tui++/Button.h>
#include <tui++/ToggleButton.h>
#include <tui++/CheckBox.h>
#include <tui++/RadioButton.h>
#include <tui++/Char.h>
#include <tui++/CharIterator.h>
#include <tui++/Color.h>
#include <tui++/Component.h>
#include <tui++/Graphics.h>
#include <tui++/Icon.h>
#include <tui++/Insets.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>

#include <cassert>
#include <optional>

namespace tui::laf {

namespace {

// The kind of indicator the button draws in front of its label, or NONE for
// plain buttons and toggles.
IndicatorKind indicator_kind(Component const *c) {
  if (is_a<CheckBox>(c)) {
    return IndicatorKind::CHECKBOX;
  } else if (is_a<RadioButton>(c)) {
    return IndicatorKind::RADIO;
  }
  return IndicatorKind::NONE;
}

// The theme property prefix of the button kind: "Button", "ToggleButton",
// "CheckBox" or "RadioButton" (the way Swing's BasicButtonUI uses the
// getPropertyPrefix() of the button's UI class).
std::string property_prefix(Component const *c) {
  if (is_a<CheckBox>(c)) {
    return "CheckBox";
  } else if (is_a<RadioButton>(c)) {
    return "RadioButton";
  } else if (is_a<ToggleButton>(c)) {
    return "ToggleButton";
  }
  return "Button";
}

// Whether the button's model is in a pressed or selected state that the
// delegate paints as "down": a plain Button while it is pressed, a toggle
// (ToggleButton/CheckBox/RadioButton) while pressed or selected.
bool paints_pressed(AbstractButton const *button) {
  auto const &model = button->get_model();
  if (model->is_pressed() and model->is_armed()) {
    return true;
  }
  return (indicator_kind(button) != IndicatorKind::NONE or is_a<ToggleButton>(button)) and model->is_selected();
}

// The color the "down" state fills the content area with: the background,
// darkened like Swing's pressed button face. Falls back to the system
// control color when the button has no background.
Color pressed_color(AbstractButton const *button) {
  auto background = button->get_background_color().value_or(Color { 0xC0, 0xC0, 0xC0 });
  return background.darker();
}

// The color of the button's label: the button's own foreground when one is
// installed, else the system control text color (the theme's
// "<Prefix>.ForegroundColor" defaults are installed onto the button by
// install_ui, so this fallback only fires before that or without a theme).
Color text_color(AbstractButton const *button) {
  return button->get_foreground_color().value_or(Color { 0, 0, 0 });
}

// Width of `text` in the screen's units; the widest line counts.
int text_width(TextMetrics const *metrics, std::string const &text) {
  return text.empty() ? 0 : metrics->get_width(text);
}

// Draws `text` starting at (x, y), clipping at the content area's right edge
// `limit`. `focus_underline` underlines the drawn glyphs (the keyboard focus
// marker of a button whose focus is painted). The mnemonic letter is
// overlaid bold+underlined, the way menu items show theirs.
void paint_clipped_text(Graphics &g, TextMetrics const *metrics, std::string const &text, int x, int y, int limit, AbstractButton const *button, bool focus_underline) {
  if (text.empty()) {
    return;
  }
  auto mnemonic_index = button->get_mnemonic().get_code() != 0 ? button->get_displayed_mnemonic_index() : -1;
  auto pos = std::string::size_type { 0 };
  auto cell = x;
  for (auto it = to_chars(text), last = end(it); it != last; ++it) {
    auto glyph = *it;
    auto width = metrics->get_char_width(glyph.get_code());
    if (cell + width > limit) {
      break;
    }
    auto is_mnemonic = mnemonic_index >= 0 and pos == std::string::size_type(mnemonic_index);
    auto attributes = is_mnemonic ? std::optional<Attributes> { Attribute::BOLD | Attribute::UNDERLINE } : (focus_underline ? std::optional<Attributes> { Attribute::UNDERLINE } : std::nullopt);
    g.draw_char(glyph, cell, y, attributes);
    cell += width;
    pos += glyph.byte_length();
  }
}

}

void ButtonUI::install_ui(std::shared_ptr<Component> const &c) {
  auto const &prefix = property_prefix(c.get());

  // The button family is opaque and filled (Swing's opaque default), on the
  // theme's "<Prefix>.BackgroundColor"/"ForegroundColor" and bordered by the
  // theme's "<Prefix>.Border" (a raised/lowered bezel that reflects the
  // model's pressed state). CheckBox/RadioButton turn their borders off in
  // their constructors and paint only the indicator, so the theme's border
  // keys of those kinds are intentionally left undefined.
  LookAndFeel::install(c.get(), "Opaque", LookAndFeel::get<bool>(prefix + ".Opaque", true));
  LookAndFeel::install_colors(c.get(), prefix + ".BackgroundColor", prefix + ".ForegroundColor");
  LookAndFeel::install_border(c.get(), prefix + ".Border");
}

std::optional<Dimension> ButtonUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto metrics = screen.get_text_metrics();
  auto margin = button->get_margin().value_or(Insets { 0, 0, 0, 0 });

  auto width = text_width(metrics.get(), button->get_text());
  auto kind = indicator_kind(c.get());
  if (kind != IndicatorKind::NONE) {
    width += indicator_column_width(*metrics);
  }
  if (auto const &icon = button->get_icon()) {
    width += int(button->get_icon_text_gap()) + icon->get_icon_width();
  }

  auto insets = c->get_insets();
  auto height = metrics->get_line_height();
  return Dimension { insets.left + margin.left + width + margin.right + insets.right,
                     insets.top + margin.top + height + margin.bottom + insets.bottom };
}

void ButtonUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto metrics = screen.get_text_metrics();

  // The content area: the component inside its border (insets) and the
  // button's margin. The border itself was painted by the component (see
  // Component::paint_border); like Swing's BasicButtonUI this delegate
  // paints the content area only.
  auto insets = c->get_insets();
  auto margin = button->get_margin().value_or(Insets { 0, 0, 0, 0 });
  auto x0 = insets.left + margin.left;
  auto y0 = insets.top + margin.top;
  auto x1 = c->get_width() - insets.right - margin.right;
  auto y1 = c->get_height() - insets.bottom - margin.bottom;
  if (x1 <= x0 or y1 <= y0) {
    return;
  }

  // The "down" state of a pressed button and of a selected toggle fills the
  // content area with the darkened face color (the raised bezel of the
  // border is painted separately and turns sunken while pressed).
  auto const &model = button->get_model();
  if (paints_pressed(button) and model->is_enabled()) {
    g.set_background_color(pressed_color(button));
    g.fill_rect(x0, y0, x1 - x0, y1 - y0);
  }

  g.set_foreground_color(text_color(button));

  // Vertically center the single label line in the content area.
  auto y = y0 + std::max(0, (y1 - y0 - metrics->get_line_height()) / 2);

  // Lay the indicator, the icon and the label out on one line (Swing's
  // layoutCompoundLabel for the default vertical CENTER / text TRAILING).
  auto kind = indicator_kind(c.get());
  auto text = button->get_text();
  auto icon_gap = int(button->get_icon_text_gap());
  auto width_so_far = 0;
  if (kind != IndicatorKind::NONE) {
    width_so_far += indicator_column_width(*metrics);
  }
  if (auto const &icon = button->get_icon()) {
    width_so_far += (width_so_far != 0 ? icon_gap : 0) + icon->get_icon_width();
  }
  auto label_width = text_width(metrics.get(), text);
  auto content_width = width_so_far + (width_so_far != 0 and not text.empty() ? icon_gap : 0) + label_width;

  // Horizontal alignment within the content area.
  auto available = x1 - x0;
  auto x = x0;
  switch (button->get_horizontal_alignment()) {
  case HorizontalAlignment::CENTER:
    x = x0 + std::max(0, (available - content_width) / 2);
    break;
  case HorizontalAlignment::RIGHT:
  case HorizontalAlignment::TRAILING:
    x = x0 + std::max(0, available - content_width);
    break;
  case HorizontalAlignment::LEFT:
  case HorizontalAlignment::LEADING:
    break;
  }

  auto cursor = x;
  if (kind != IndicatorKind::NONE) {
    paint_indicator(g, *metrics, kind, cursor, y, model->is_selected());
    cursor += indicator_column_width(*metrics);
  }
  if (auto const &icon = button->get_icon()) {
    if (cursor != x) {
      cursor += icon_gap;
    }
    icon->paint_icon(c.get(), g, cursor, y);
    cursor += icon_gap + icon->get_icon_width();
  }
  if (not text.empty()) {
    if (cursor != x) {
      cursor += icon_gap;
    }
    auto focus_underline = button->is_focus_owner() and button->is_focus_painted() and not model->is_selected();
    paint_clipped_text(g, metrics.get(), text, cursor, y, x1, button, focus_underline);
  }
}

}
