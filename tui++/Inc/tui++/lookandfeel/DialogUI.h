#pragma once

#include <tui++/Component.h>
#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui::laf {

// The dialog's delegate: the dialog chrome a JDialog gets from its look-and-
// feel -- an opaque face on the system control colors and the drop shadow the
// theme defines for dialogs (Window installs "Dialog.Shadow"). The border
// belongs to the root pane, which owns the window decorations (Dialog::init
// installs it) and insets the content with it.
class DialogUI: public ComponentUI {
public:
  virtual void install_ui(std::shared_ptr<Component> const &c) override {
    ComponentUI::install_ui(c);

    // A window is opaque when it has a background color (Window::is_opaque);
    // installing one gives the dialog a face of its own instead of letting
    // the windows below show through it, which is what makes the shadow and
    // the dialog's own rectangle read as one surface.
    LookAndFeel::install_colors(c.get(), "Dialog.BackgroundColor", "Dialog.ForegroundColor");
  }
};

}
