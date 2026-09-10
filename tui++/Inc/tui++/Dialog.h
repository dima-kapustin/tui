#pragma once

#include <tui++/Frame.h>

namespace tui {

namespace laf {
class DialogUI;
}

// Dialog - Swing's JDialog: a window owned by a frame (or by another dialog)
// that floats over it, packs itself to its content and, when modal, holds the
// input of the windows it stands over until it is dismissed.
//
// The modality follows java.awt.Dialog.ModalityType:
//
//   * MODELESS          the dialog blocks nothing (the default, as for
//                       Swing's JDialog; set_modal(true) makes it
//                       APPLICATION_MODAL the way the JDialog(owner, true)
//                       constructor does),
//   * DOCUMENT_MODAL    blocks the other windows of the dialog's document --
//                       the frame it belongs to and everything owned by it,
//   * APPLICATION_MODAL blocks every other window of the application (in a
//                       single-application toolkit the same set as
//                       TOOLKIT_MODAL),
//   * TOOLKIT_MODAL     same as APPLICATION_MODAL here (the port has one
//                       toolkit and one screen).
//
// A modal dialog's own windows -- the popups it opens, the dialogs it owns --
// are never blocked by it, so a context menu or a combo box dropdown of the
// dialog keeps working while it is up; the dialog's children are the one
// exception Java's modality makes too.
//
// show() (set_visible(true)) of a modal dialog does not return until the
// dialog is hidden: the screen pumps events in a nested loop while the dialog
// is up, the way Swing's Dialog.setVisible(true) keeps the event dispatch
// thread pumping. A program that shows a modal dialog from a listener (the
// usual thing, e.g. a menu item) gets the Swing behavior: the call returns
// after the dialog is dismissed.

class Dialog: public Window {
  using base = Window;

public:
  // java.awt.Dialog.ModalityType: what the dialog blocks while it is up.
  enum class ModalityType {
    MODELESS,
    DOCUMENT_MODAL,
    APPLICATION_MODAL,
    TOOLKIT_MODAL
  };

private:
  Property<ModalityType> modality_type { this, "ModalityType", ModalityType::MODELESS };

public:
  // Swing's JDialog.setModal / isModal, kept alongside the ModalityType the
  // way java.awt.Dialog keeps them: the program action behind the (owner,
  // true) constructor is set_modal(true).
  void set_modal(bool value) {
    set_modality_type(value ? ModalityType::APPLICATION_MODAL : ModalityType::MODELESS);
  }

  bool is_modal() const {
    return get_modality_type() != ModalityType::MODELESS;
  }

  ModalityType get_modality_type() const {
    return this->modality_type;
  }

  void set_modality_type(ModalityType value) {
    this->modality_type = value;
  }

  // Whether this dialog blocks input to `window` while it is showing: the
  // rule behind Window::is_modal_blocked and the screen's modal filtering
  // (Swing's ModalEventFilter).
  virtual bool blocks(const std::shared_ptr<Window> &window) const override;

  // Swing's JDialog.dispose: takes the dialog down and lets it be collected.
  // A dialog shown again afterwards is shown from scratch (it packs itself
  // again, exactly as a dialog that was never sized).
  void dispose();

  // Swing's setLocationRelativeTo: centers the dialog over `c`, or over its
  // owner when `c` is null; a null owner and an unsized dialog leave the
  // location alone (Swing centers such a dialog on the screen).
  void set_location_relative_to(std::shared_ptr<Component> const &c);

  std::shared_ptr<laf::DialogUI> get_ui() const;

protected:
  Dialog(std::shared_ptr<Dialog> const &owner);
  Dialog(std::shared_ptr<Frame> const &owner);
  Dialog(std::shared_ptr<Window> const &owner);
  Dialog(std::shared_ptr<Window> const &owner, std::string const &title, ModalityType modality_type = ModalityType::MODELESS);

  // A dialog floats over the frame that owns it and shades it, like Swing's
  // dialogs do (the theme defines "Dialog.Shadow" for them).
  virtual std::string_view get_shadow_key() const override {
    return "Dialog.Shadow";
  }

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;
  virtual std::shared_ptr<laf::ComponentUI> create_ui() override;

  // Swing's setVisible(true) for a dialog: a dialog without a size of its own
  // packs itself first, and a modal dialog keeps the caller in a nested event
  // pump until it is hidden (see the class comment).
  virtual void show() override;
};

}
