#include <tui++/terminal/text/TextTheme.h>

#include <tui++/border/LineBorder.h>
#include <tui++/border/BevelBorder.h>
#include <tui++/border/EmptyBorder.h>
#include <tui++/border/EtchedBorder.h>
#include <tui++/border/CompoundBorder.h>

#include <tui++/lookandfeel/SystemColorKeys.h>
#include <tui++/lookandfeel/basic/MarginBorder.h>
#include <tui++/lookandfeel/basic/ButtonBorder.h>

#include <tui++/Insets.h>
#include <tui++/Shadow.h>

#include <tui++/Font.h>

namespace tui {

void TextTheme::init() {
  put( //
      { { "Menu.SubmenuPopupOffsetX", 0 }, //
        { "Menu.SubmenuPopupOffsetY", 0 }, //
        { "Menu.MenuPopupOffsetX", 0 }, //
        { "Menu.MenuPopupOffsetY", 0 }, //
      });

  // The default font used by the graphic (sixel) screen: the size is the
  // glyph width in pixels, the style selects bold/italic. The text screen
  // measures in cells and ignores it.
  put("defaultFont", Font { });

  // Menu label margins, in cells, like Swing's "MenuItem.margin" /
  // "Menu.margin" (the graphic theme overrides them in pixels).
  put("MenuItem.margin", make_resource<Insets>(0, 1, 0, 1));
  put("Menu.margin", make_resource<Insets>(0, 1, 0, 1));

  init_system_color_defaults();
  init_component_defaults();
}

void TextTheme::init_system_color_defaults() {
  update_system_colors();

  using namespace SystemColorKeys;
  for (auto &&key : SYSTEM_COLOR_KEYS) {
    put(key, make_resource(get_system_color(key)));
  }
}

void TextTheme::init_component_defaults() {
  auto red = make_resource(RED_COLOR);
  auto black = make_resource(BLACK_COLOR);
  auto white = make_resource(WHITE_COLOR);
  auto yellow = make_resource(YELLOW_COLOR);
  auto gray = make_resource(GRAY_COLOR);
  auto lightGray = make_resource(LIGHT_GRAY_COLOR);
  auto darkGray = make_resource(DARK_GRAY_COLOR);
  auto scrollBarTrack = make_resource<Color>(224, 224, 224);

  auto control = get_color(SystemColorKeys::CONTROL);
  auto controlDkShadow = get_color(SystemColorKeys::CONTROL_DK_SHADOW);
  auto controlHighlight = get_color(SystemColorKeys::CONTROL_HIGHLIGHT);
  auto controlLtHighlight = get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT);
  auto controlShadow = get_color(SystemColorKeys::CONTROL_SHADOW);
  auto controlText = get_color(SystemColorKeys::CONTROL_TEXT);
  auto menu = get_color(SystemColorKeys::MENU);
  auto menuText = get_color(SystemColorKeys::MENU_TEXT);
  auto textHighlight = get_color(SystemColorKeys::TEXT_HIGHLIGHT);
  auto textHighlightText = get_color(SystemColorKeys::TEXT_HIGHLIGHT_TEXT);
  auto textInactiveText = get_color(SystemColorKeys::TEXT_INACTIVE_TEXT);
  auto textText = get_color(SystemColorKeys::TEXT_TEXT);
  auto window = get_color(SystemColorKeys::WINDOW);

  // Hovered/selected menu text (both top-level menus and popup menu items)
  // is painted on these colors; the graphic theme inherits them.
  put("MenuItem.SelectionBackground", textHighlight.value_or(Color { 0, 0, 128 }));
  put("MenuItem.SelectionForeground", textHighlightText.value_or(WHITE_COLOR));

  // Swing's menu chrome: the menu bar and popup menu are painted on the
  // system menu background and their items on the system menu text color
  // (BasicMenuBarUI / BasicPopupMenuUI / BasicMenuItemUI install these as
  // "MenuBar.background/foreground", "PopupMenu.background/foreground" and
  // "MenuItem.background/foreground"). Without them an opaque menu bar or
  // popup paints no background, so its items float over the frame beneath it
  // and black text would be unreadable on a dark frame. Every key is a theme
  // property, so a program can override any of them (the UIManager.put
  // equivalent) or call set_background_color / set_foreground_color directly.
  put("MenuBar.BackgroundColor", menu.value_or(Color { 0xC0, 0xC0, 0xC0 }));
  put("MenuBar.ForegroundColor", menuText.value_or(BLACK_COLOR));
  put("PopupMenu.BackgroundColor", menu.value_or(Color { 0xC0, 0xC0, 0xC0 }));
  put("PopupMenu.ForegroundColor", menuText.value_or(BLACK_COLOR));
  put("MenuItem.BackgroundColor", menu.value_or(Color { 0xC0, 0xC0, 0xC0 }));
  put("MenuItem.ForegroundColor", menuText.value_or(BLACK_COLOR));

  auto zero_insets = make_resource<Insets>(0, 0, 0, 0);
  auto two_insets = make_resource<Insets>(2, 2, 2, 2);
  auto three_insets = make_resource<Insets>(3, 3, 3, 3);

  auto margin_border = BorderFactory { [this] {
    return make_shared_resource<laf::MarginBorder>();
  } };

  auto etched_border = BorderFactory { [this] {
    return make_shared_resource<EtchedBorder>();
  } };

  auto lowered_bevel_border = BorderFactory { [this] {
    return make_shared_resource<BevelBorder>(BevelBorder::LOWERED);
  } };

  auto internal_frame_border = BorderFactory { [this] {
    static auto border = make_shared_resource<CompoundBorder>( //
        std::make_shared<BevelBorder>( //
            BevelBorder::RAISED, //
            get_color("InternalFrame.BorderLight"), //
            get_color("InternalFrame.BorderHighlight"), //
            get_color("InternalFrame.BorderDarkShadow"), //
            get_color("InternalFrame.BorderShadow")), //
        std::make_shared<LineBorder>( //
            Stroke::LIGHT, //
            get_color("InternalFrame.BorderColor")));
    return border;
  } };

  auto black_line_border = BorderFactory { [this] {
    return make_shared_resource<LineBorder>(Stroke::LIGHT, BLACK_COLOR);
  } };

  auto popup_menu_border = internal_frame_border;

  auto focusCellHighlightBorder = BorderFactory { [this] {
    return make_shared_resource<LineBorder>(Stroke::LIGHT, YELLOW_COLOR);
  } };

  auto noFocusBorder = make_shared_resource<EmptyBorder>(1, 1, 1, 1);

  auto tableHeaderBorder = BorderFactory { [this] {
    return make_shared_resource<BevelBorder>( //
        BevelBorder::RAISED, //
        get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT), //
        get_color(SystemColorKeys::CONTROL), //
        get_color(SystemColorKeys::CONTROL_DK_SHADOW), //
        get_color(SystemColorKeys::CONTROL_SHADOW));
  } };

  auto button_border = BorderFactory { [this] {
    return make_shared_resource<laf::ButtonBorder>( //
        get_color(SystemColorKeys::CONTROL_SHADOW), //
        get_color(SystemColorKeys::CONTROL_DK_SHADOW), //
        get_color(SystemColorKeys::CONTROL_HIGHLIGHT), //
        get_color(SystemColorKeys::CONTROL_LT_HIGHLIGHT));
  } };

  put("Button.Border", button_border);

  // The button family's padding inside its bezel: Swing's "Button.margin"
  // (the Metal default is a wide {2, 14, 2, 14}; one cell around the label is
  // the character-cell equivalent). The check box and radio button kinds have
  // no bezel -- their label follows the indicator's own gap column -- so they
  // take no margin.
  put("Button.margin", make_resource<Insets>(0, 1, 0, 1));
  put("ToggleButton.margin", make_resource<Insets>(0, 1, 0, 1));

  // The button family (Swing's BasicButtonUI default properties). Every
  // button kind installs its background/foreground from its own prefix, so a
  // program can restyle one kind ("CheckBox.BackgroundColor") without
  // touching the others; the defaults all share the system control colors.
  put("ToggleButton.Border", button_border);
  for (auto &&prefix : { "Button", "ToggleButton", "CheckBox", "RadioButton", "ComboBox" }) {
    put(std::string(prefix) + ".BackgroundColor", control);
    put(std::string(prefix) + ".ForegroundColor", controlText);
  }

  // The combo box reserves one cell around its field; the check box and radio
  // button labels sit directly next to their indicator.
  put("ComboBox.margin", make_resource<Insets>(0, 1, 0, 1));

  // Drop shadows (see Shadow): the floating windows -- a popup menu (a menu
  // bar's dropdown and every submenu), a combo box dropdown and a dialog --
  // darken the cells beneath their right and bottom edge by half, so they
  // read as sitting above the window below. A frame fills the screen and
  // casts none; its key exists so a look-and-feel can shade a frame that does
  // not cover the whole screen. The global switch turns them all off.
  auto popup_shadow = make_resource<Shadow>(BLACK_COLOR, 0.5, Point { 2, 1 });
  put("Shadow.Enabled", true);
  put("PopupMenu.Shadow", popup_shadow);
  put("ComboBox.Shadow", popup_shadow);
  put("Dialog.Shadow", popup_shadow);

  // Swing's text components (BasicTextUI defaults): a field paints the system
  // window colors and its selection the system text-highlight pair; the
  // margin is the one cell Swing's TextField.margin reserves around the text
  // (the editable combo's editor shares it through the key's default).
  put("TextField.BackgroundColor", window);
  put("TextField.ForegroundColor", textText);
  put("TextField.SelectionBackground", textHighlight);
  put("TextField.SelectionForeground", textHighlightText);
  put("TextField.margin", make_resource<Insets>(0, 1, 0, 1));

//  put( { { "MenuItem.border", margin_border } });
//
//  auto border = get_border("MenuItem.border");

}

}
