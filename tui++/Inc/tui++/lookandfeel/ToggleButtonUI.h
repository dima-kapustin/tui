#pragma once

#include <tui++/lookandfeel/ButtonUI.h>

namespace tui::laf {

// Swing's BasicToggleButtonUI. Toggleable buttons (ToggleButton and its
// CheckBox/RadioButton subclasses) paint through the shared BasicButtonUI
// delegate; this class exists so the look-and-feel can hand them a distinct
// UI class (and theme property prefix).
class ToggleButtonUI: public ButtonUI {
  using base = ButtonUI;
};

}
