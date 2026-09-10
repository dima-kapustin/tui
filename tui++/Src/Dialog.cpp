#include <tui++/Dialog.h>

#include <tui++/RootPane.h>
#include <tui++/Screen.h>

#include <tui++/lookandfeel/DialogUI.h>
#include <tui++/lookandfeel/LookAndFeel.h>

namespace tui {

Dialog::Dialog(std::shared_ptr<Dialog> const &owner) :
    Dialog(std::static_pointer_cast<Window>(owner)) {
}

Dialog::Dialog(std::shared_ptr<Frame> const &owner) :
    Dialog(std::static_pointer_cast<Window>(owner)) {
}

Dialog::Dialog(std::shared_ptr<Window> const &owner) :
    base(owner) {
}

Dialog::Dialog(std::shared_ptr<Window> const &owner, std::string const &title, ModalityType modality_type) :
    base(owner) {
  set_title(title);
  set_modality_type(modality_type);
}

void Dialog::init() {
  base::init();

  // A dialog takes keys (its default button, its Escape) and window events
  // (the activation a modal dialog changes); mouse events are enabled by the
  // components that listen for them, as everywhere else.
  enable_events(KEY_EVENT_MASK | WINDOW_EVENT_MASK);

  // Swing's JDialog decoration: the root pane knows it belongs to a dialog
  // (the "plain dialog" style), which a look-and-feel may paint a title bar
  // for, and the dialog's own face, border and shadow come from the theme.
  this->root_pane->set_window_decoration_style(RootPane::PLAIN_DIALOG);

  // Swing's root pane owns the window decorations, so the dialog's border is
  // installed there: the root pane's insets are what the root layout keeps
  // the content panes inside -- the dialog's content lands inside the box --
  // and the border is painted over the root pane's face. (A frame's delegate
  // paints its border on the window itself, the older way.)
  laf::LookAndFeel::install_border(this->root_pane.get(), "Dialog.Border");

  // A dialog is focusable even before it has a focusable component: the
  // keyboard belongs to the dialog the moment it opens (its default button,
  // its Escape), while a plain window with nothing to focus is not.
  set_focusable_window_state(true);
}

std::shared_ptr<laf::ComponentUI> Dialog::create_ui() {
  return laf::LookAndFeel::create_ui(this);
}

std::shared_ptr<laf::DialogUI> Dialog::get_ui() const {
  return std::static_pointer_cast<laf::DialogUI>(this->ui.value());
}

bool Dialog::blocks(const std::shared_ptr<Window> &window) const {
  if (not is_modal() or not is_showing()) {
    return false;
  }

  if (window.get() == this) {
    // A modal dialog does not block itself.
    return false;
  }

  // The dialog's own windows -- the popups over it, the dialogs it owns -- are
  // never blocked: a context menu or a combo box dropdown of a modal dialog
  // works while it is up, the way Java's modality excludes the modal window's
  // children.
  for (auto owner = window->get_owner(); owner; owner = owner->get_owner()) {
    if (owner.get() == this) {
      return false;
    }
  }

  if (get_modality_type() == ModalityType::DOCUMENT_MODAL and get_owner()) {
    // The document is the window hierarchy the dialog belongs to: the frame
    // the dialog was created for (its owner chain's root) and everything that
    // belongs to it. Windows of another frame are not blocked -- that is what
    // separates document modality from application modality. A document-modal
    // dialog without an owner has no document to limit itself to, so it blocks
    // the whole application (Java resolves the same case the same way).
    auto document_root = [](Window const *window) {
      while (auto owner = window->get_owner()) {
        window = owner.get();
      }
      return window;
    };
    return document_root(window.get()) == document_root(this);
  }

  // APPLICATION_MODAL and TOOLKIT_MODAL: every other window of the
  // application (one screen, one toolkit) except the dialog's own children,
  // which the check above already let through.
  return true;
}

void Dialog::dispose() {
  // Swing's dispose takes the window down for good; the port has no native
  // resources to release, so hiding the dialog is the whole of it. The
  // dialog keeps the size it was given (or packed to); a program that wants
  // the next show to size it to its content again clears the size.
  set_visible(false);
}

void Dialog::set_location_relative_to(std::shared_ptr<Component> const &c) {
  auto relative = c;
  if (not relative) {
    relative = get_owner();
  }
  if (not relative) {
    return;
  }

  auto origin = relative->get_location_on_screen();
  auto size = relative->get_size();
  set_location(origin.x + (size.width - get_width()) / 2, origin.y + (size.height - get_height()) / 2);
}

void Dialog::show() {
  if (get_width() <= 0 or get_height() <= 0) {
    // Swing's setVisible(true) packs a dialog that has no size of its own:
    // the dialog takes the size its content asks for. A dialog the program
    // sized (or packed) keeps the size it has.
    pack();
  }

  base::show();

  if (is_modal() and is_showing()) {
    // The dialog is up and holds the input of the windows it stands over;
    // show() does not return until it is hidden (Swing's modal dialogs block
    // their caller the same way, with a nested event pump on the dispatch
    // thread). Events that arrived between the show and the pump -- the
    // activation of the dialog, a button's paint -- are dispatched by the
    // pump, in order, as the running event loop would.
    screen.run_modal_event_loop(std::dynamic_pointer_cast<Window>(shared_from_this()));
  }
}

}
