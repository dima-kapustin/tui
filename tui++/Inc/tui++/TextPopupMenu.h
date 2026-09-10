#pragma once

// TextPopupMenu - the "standard" context menu of the toolkit's text views:
// the menu a text component shows on the popup trigger (Swing's
// JTextComponent popup, which BasicTextUI builds from the DefaultEditorKit
// actions).
//
// The rows are the Swing set, in the Swing order:
//
//     Undo
//     Redo
//     ------
//     Cut
//     Copy
//     Paste
//     Delete
//     ------
//     Select All
//
// Every row is bound to an action that asks the text component whether it can
// run right now -- Undo is dead with an empty history, Cut with no selection
// or on a read-only view, Paste with an empty Clipboard -- and the answers are
// refreshed whenever the menu is about to show, the way Swing's TextActions
// consult the target's EditorKit on each popup.
//
// TextField and TextArea install one for themselves when they are created
// (see their init), so a program gets the standard menu for free and replaces
// it by calling set_component_popup_menu with its own menu.

#include <tui++/AbstractAction.h>
#include <tui++/PopupMenu.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tui {

class TextComponent;

namespace detail {

// One row's action. The command is a pair of functors over the text component
// (Swing's TextAction and its EditorKit split the same way), and the
// enablement is re-asked whenever the menu is about to show.
class TextPopupAction: public AbstractAction {
  std::weak_ptr<TextComponent> const target;
  std::function<bool(TextComponent const&)> const can_do;
  std::function<void(TextComponent&)> const do_it;

public:
  TextPopupAction(std::string const &name, std::shared_ptr<TextComponent> const &target, std::function<bool(TextComponent const&)> can_do, std::function<void(TextComponent&)> do_it) :
      target(target), can_do(std::move(can_do)), do_it(std::move(do_it)) {
    set_name(name);
  }

  // Publishes the component's answer to "can the command run now?" as the
  // action's enabled state. The action is the MenuItem's action, so the
  // change reaches the row and the menu paints it grayed out (see
  // AbstractButton::action_property_changed).
  void refresh() {
    if (auto target = this->target.lock()) {
      set_enabled(this->can_do(*target));
    } else {
      set_enabled(false);
    }
  }

  virtual void action_performed(ActionEvent &) override {
    if (auto target = this->target.lock()) {
      this->do_it(*target);
    }
  }
};

}

class TextPopupMenu: public PopupMenu {
  std::weak_ptr<TextComponent> target;
  std::vector<std::shared_ptr<detail::TextPopupAction>> actions;

protected:
  TextPopupMenu(std::shared_ptr<TextComponent> const &target);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;

private:
  // Asks the text component what it can do right now and publishes the
  // answers to the rows' actions.
  void refresh_actions();
};

std::shared_ptr<TextPopupMenu> create_text_popup_menu(std::shared_ptr<TextComponent> const &target);

}
