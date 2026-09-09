#pragma once

#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui::laf {

// The UI of the separator rows between groups of a popup menu (Swing's
// BasicPopupMenuSeparatorUI). It paints a thin horizontal rule, inset by the
// theme's "PopupMenuSeparator.margin" (one cell on the text screen, the
// pixel margin of SixelTheme on the graphic screen), and sizes itself to a
// popup item row so the menu's groups stay visibly separated.
class PopupMenuSeparatorUI: public ComponentUI {
public:
  virtual std::optional<Dimension> get_preferred_size(std::shared_ptr<const Component> const &c) const override;

protected:
  virtual void paint(Graphics &g, std::shared_ptr<const Component> const &c) const override;
};

}
