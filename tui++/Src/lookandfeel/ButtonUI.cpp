#include <tui++/lookandfeel/ButtonUI.h>

#include <tui++/lookandfeel/LazyActionMap.h>
#include <tui++/lookandfeel/LookAndFeel.h>
#include <tui++/lookandfeel/Translations.h>
#include <tui++/lookandfeel/basic/ToggleIndicator.h>

#include <tui++/InputMap.h>
#include <tui++/KeyStroke.h>
#include <tui++/event/KeyEvent.h>

#include <tui++/AbstractButton.h>
#include <tui++/Button.h>
#include <tui++/Switch.h>
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
#include <tui++/Shadow.h>
#include <tui++/TextMetrics.h>

#include <cassert>
#include <chrono>
#include <optional>

namespace tui::laf {

// The theme keys of the button family's keyboard resources. The four button
// kinds take the same gestures, so one pair of maps serves them all (Swing's
// "Button.actionMap" and "Button.focusInputMap" are shared the same way).
constexpr std::string_view ACTION_MAP_KEY = "Button.ActionMap";
constexpr std::string_view FOCUS_INPUT_MAP_KEY = "Button.FocusInputMap";

// The command of the keyboard click, the way the menu items name theirs.
constexpr std::string CLICK = "do_click";

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
// "CheckBox", "RadioButton" or "Switch" (the way Swing's BasicButtonUI uses
// the getPropertyPrefix() of the button's UI class).
std::string property_prefix(Component const *c) {
  if (is_a<CheckBox>(c)) {
    return "CheckBox";
  } else if (is_a<RadioButton>(c)) {
    return "RadioButton";
  } else if (is_a<Switch>(c)) {
    return "Switch";
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

// Draws `text` starting at (x, y), clipping at the content area's right edge
// `limit`. `focus_underline` underlines the drawn glyphs (the keyboard focus
// marker of a button whose focus is painted). The mnemonic letter is
// overlaid bold+underlined, the way menu items show theirs.
void paint_clipped_text(Graphics &g, TextMetrics const *metrics, std::string const &text, int x, int y, int limit, AbstractButton const *button, bool focus_underline) {
  if (text.empty()) {
    return;
  }

  // The mnemonic underline: the index the button tracks, while the label drawn
  // is the program's own text; a translated label is searched for the letter
  // instead (a translation may move it, or drop it altogether).
  auto mnemonic_index = -1;
  if (button->get_mnemonic().get_code() != 0) {
    if (text == button->get_text()) {
      mnemonic_index = button->get_displayed_mnemonic_index();
    } else if (auto pos = text.find(button->get_mnemonic()); pos != text.npos) {
      mnemonic_index = int(pos);
    }
  }

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
  this->button = static_cast<AbstractButton*>(c.get());
  auto const &prefix = property_prefix(c.get());

  // The button family is opaque and filled (Swing's opaque default), on the
  // theme's "<Prefix>.BackgroundColor"/"ForegroundColor" and bordered by the
  // theme's "<Prefix>.Border" (a raised/lowered bezel that reflects the
  // model's pressed state). CheckBox/RadioButton turn their borders off in
  // their constructors and paint only the indicator, so the theme's border
  // keys of those kinds are intentionally left undefined.
  LookAndFeel::install(c.get(), "Opaque", LookAndFeel::get<bool>(prefix + ".Opaque", true));
  LookAndFeel::install(c.get(), "Margin", LookAndFeel::get<std::optional<Insets>>(prefix + ".margin"));
  LookAndFeel::install_colors(c.get(), prefix + ".BackgroundColor", prefix + ".ForegroundColor");
  LookAndFeel::install_border(c.get(), prefix + ".Border");

  // The shadow the button may cast: the theme's "<Prefix>.Shadow" unless the
  // program set one (see AbstractButton::get_shadow). The button's border
  // reserves its room, so a shadow widens the button's box.
  LookAndFeel::install(c.get(), "Shadow", LookAndFeel::get<std::optional<Shadow>>(prefix + ".Shadow"));

  install_listeners();
  install_keyboard_actions();
}

void ButtonUI::uninstall_ui(std::shared_ptr<Component> const &c) {
  uninstall_keyboard_actions();
  uninstall_listeners();
  this->button = nullptr;
}

void ButtonUI::install_listeners() {
  this->button->add_listener(MousePressEvent::MOUSE_PRESSED, this->mouse_pressed_listener);
  this->button->add_listener(MousePressEvent::MOUSE_RELEASED, this->mouse_released_listener);
  this->button->add_listener(this->mouse_overed_listener);
}

void ButtonUI::uninstall_listeners() {
  this->button->remove_listener(this->mouse_overed_listener);
  this->button->remove_listener(this->mouse_released_listener);
  this->button->remove_listener(this->mouse_pressed_listener);
}

void ButtonUI::install_keyboard_actions() {
  install_lazy_action_map();
  install_focus_input_map();
}

void ButtonUI::uninstall_keyboard_actions() {
  // The maps are theme resources parented by the button's own (empty) maps:
  // both go away with the button, and the theme keeps the shared resources
  // for the next one, the way a Swing UIManager keeps its action and input
  // maps across components.
}

void ButtonUI::install_lazy_action_map() {
  auto action_map = LookAndFeel::get<std::shared_ptr<ActionMap>>(ACTION_MAP_KEY);
  if (not action_map) {
    action_map = std::make_shared<LazyActionMap>(load_action_map);
    LookAndFeel::put(ACTION_MAP_KEY, action_map);
  }
  LookAndFeel::replace_action_map(this->button, action_map);
}

void ButtonUI::load_action_map(LazyActionMap &map) {
  // Space clicks the focused button. Swing's BasicButtonUI binds the key's
  // press and release to the model separately; a terminal reports no key
  // release, so the one stroke does the whole click -- what a mouse release
  // over the button does -- and the model fires its action exactly once.
  map.emplace(CLICK, [](ActionEvent &e) {
    std::static_pointer_cast<AbstractButton>(e.source)->do_click(std::chrono::milliseconds::zero());
  });
}

void ButtonUI::install_focus_input_map() {
  auto input_map = LookAndFeel::get<std::shared_ptr<InputMap>>(FOCUS_INPUT_MAP_KEY);
  if (not input_map) {
    input_map = LookAndFeel::make_theme_resource<InputMap>();
    input_map->emplace(KeyStroke(KeyEvent::VK_SPACE, InputEvent::NO_MODIFIERS), CLICK);
    LookAndFeel::put(FOCUS_INPUT_MAP_KEY, input_map);
  }
  LookAndFeel::replace_input_map(this->button, Component::WHEN_FOCUSED, input_map);
}

void ButtonUI::mouse_pressed(MousePressEvent &e) {
  if (e.id != MousePressEvent::MOUSE_PRESSED or e.button != MouseEvent::LEFT_BUTTON or not this->button->is_enabled()) {
    return;
  }

  // The press takes the focus (a mouse event, so a radio button's group does
  // not redirect it to its selected member the way traversal does) and puts
  // the model into the armed+pressed state the delegate paints as "down".
  this->button->request_focus(FocusEvent::Cause::MOUSE_EVENT);

  auto const &model = this->button->get_model();
  model->set_armed(true);
  model->set_pressed(true);
}

void ButtonUI::mouse_released(MousePressEvent &e) {
  if (e.id != MousePressEvent::MOUSE_RELEASED or e.button != MouseEvent::LEFT_BUTTON) {
    return;
  }

  // Dropping the pressed state fires the model's action while the button is
  // still armed (the pointer is on it); the disarm that follows leaves a
  // release outside the button without an action.
  auto const &model = this->button->get_model();
  model->set_pressed(false);
  model->set_armed(false);
}

void ButtonUI::mouse_overed(MouseOverEvent &e) {
  auto const &model = this->button->get_model();
  if (not model->is_pressed() or not this->button->is_enabled()) {
    return;
  }

  // While the button is held, leaving it disarms and coming back arms again:
  // the "down" look follows the pointer, and only a release over the button
  // fires its action.
  model->set_armed(e.type() == MouseOverEvent::MOUSE_ENTERED);
}

std::optional<Dimension> ButtonUI::get_preferred_size(std::shared_ptr<const Component> const &c) const {
  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto metrics = screen.get_text_metrics();
  auto margin = button->get_margin().value_or(Insets { 0, 0, 0, 0 });

  auto width = content_width(*metrics, *c);

  auto insets = c->get_insets();
  auto height = metrics->get_line_height();
  return Dimension { insets.left + margin.left + width + margin.right + insets.right,
                     insets.top + margin.top + height + margin.bottom + insets.bottom };
}

void ButtonUI::paint(Graphics &g, std::shared_ptr<const Component> const &c) const {
  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto area = get_content_area(c);
  if (area.empty()) {
    return;
  }

  // The "down" state of a pressed button and of a selected toggle fills the
  // content area with the darkened face color (the raised bezel of the border
  // is painted separately and turns sunken while pressed).
  auto const &model = button->get_model();
  if (paints_pressed(button) and model->is_enabled()) {
    g.set_background_color(pressed_color(button));
    g.fill_rect(area);
  }

  paint_content(g, c);
}

void ButtonUI::paint_content(Graphics &g, std::shared_ptr<const Component> const &c) const {
  auto area = get_content_area(c);
  if (area.empty()) {
    return;
  }

  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto metrics = screen.get_text_metrics();

  // The leading visual is drawn in the button's own foreground (see below).
  g.set_foreground_color(text_color(button));

  // Vertically center the single label line in the content area.
  auto y = area.y + std::max(0, (area.height - metrics->get_line_height()) / 2);

  // Lay the leading visual, the icon and the label out on one line (Swing's
  // layoutCompoundLabel for the default vertical CENTER / text TRAILING),
  // measuring the line exactly as the preferred size did.
  auto text = displayed_text(*button, button->get_text());
  auto icon_gap = int(button->get_icon_text_gap());
  auto content = content_width(*metrics, *c);

  // Horizontal alignment within the content area.
  auto x = area.x;
  switch (button->get_horizontal_alignment()) {
  case HorizontalAlignment::CENTER:
    x = area.x + std::max(0, (area.width - content) / 2);
    break;
  case HorizontalAlignment::RIGHT:
  case HorizontalAlignment::TRAILING:
    x = area.x + std::max(0, area.width - content);
    break;
  case HorizontalAlignment::LEFT:
  case HorizontalAlignment::LEADING:
    break;
  }

  // The leading visual comes first (the indicator, the switch track) and the
  // label follows the space it used. The icon and the label take the button's
  // own colors back: a leading visual may paint in a palette of its own (the
  // switch's track and thumb are not the button's colors), and the label must
  // not come out in the track's color or on the track's background.
  auto cursor = x + paint_leading(g, *metrics, *c, x, y);
  g.set_foreground_color(text_color(button));
  g.set_background_color(c->get_background_color());

  auto const &icon = button->get_icon();
  if (icon) {
    icon->paint_icon(c.get(), g, cursor, y);
    cursor += icon->get_icon_width();
  }
  if (not text.empty()) {
    if (icon) {
      cursor += icon_gap;
    }
    auto focus_underline = button->is_focus_owner() and button->is_focus_painted() and not button->get_model()->is_selected();
    paint_clipped_text(g, metrics.get(), text, cursor, y, area.right(), button, focus_underline);
  }
}

int ButtonUI::content_width(TextMetrics const &metrics, Component const &c) const {
  auto const &button = static_cast<AbstractButton const &>(c);

  // The leading visual (the indicator, the switch track) already ends with its
  // own gap; the button's icon-text gap separates the icon and the label when
  // both are present.
  auto width = leading_width(metrics, c);
  auto const &icon = button.get_icon();
  if (icon) {
    width += icon->get_icon_width();
  }
  if (auto text = displayed_text(button, button.get_text()); not text.empty()) {
    if (icon) {
      width += int(button.get_icon_text_gap());
    }
    width += metrics.get_width(text);
  }
  return width;
}

int ButtonUI::leading_width(TextMetrics const &metrics, Component const &c) const {
  auto kind = indicator_kind(&c);
  return kind == IndicatorKind::NONE ? 0 : indicator_column_width(metrics);
}

int ButtonUI::paint_leading(Graphics &g, TextMetrics const &metrics, Component const &c, int x, int y) const {
  auto kind = indicator_kind(&c);
  if (kind != IndicatorKind::NONE) {
    paint_indicator(g, metrics, kind, x, y, static_cast<AbstractButton const &>(c).get_model()->is_selected());
  }
  return leading_width(metrics, c);
}

Rectangle ButtonUI::get_content_area(std::shared_ptr<const Component> const &c) const {
  auto const *button = static_cast<const AbstractButton*>(c.get());
  auto insets = c->get_insets();
  auto margin = button->get_margin().value_or(Insets { 0, 0, 0, 0 });
  auto x0 = insets.left + margin.left;
  auto y0 = insets.top + margin.top;
  auto x1 = c->get_width() - insets.right - margin.right;
  auto y1 = c->get_height() - insets.bottom - margin.bottom;
  if (x1 <= x0 or y1 <= y0) {
    return { };
  }
  return { x0, y0, x1 - x0, y1 - y0 };
}

}
