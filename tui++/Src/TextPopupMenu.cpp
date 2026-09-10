#include <tui++/TextPopupMenu.h>

#include <tui++/Clipboard.h>
#include <tui++/TextComponent.h>

namespace tui {

TextPopupMenu::TextPopupMenu(std::shared_ptr<TextComponent> const &target) :
    target(target) {
}

void TextPopupMenu::init() {
  PopupMenu::init();

  // The rows are built here rather than in the constructor: adding children
  // reaches back for the menu's own shared_ptr (the hierarchy events,
  // invalidate()), which does not exist yet while the constructor runs.
  auto add_row = [this](std::string const &label, std::function<bool(TextComponent const&)> can_do, std::function<void(TextComponent&)> do_it) {
    auto action = std::make_shared<detail::TextPopupAction>(label, this->target.lock(), std::move(can_do), std::move(do_it));
    this->actions.emplace_back(action);
    add(action);
  };

  add_row("Undo", [](TextComponent const &t) { return t.can_undo(); }, [](TextComponent &t) { t.undo(); });
  add_row("Redo", [](TextComponent const &t) { return t.can_redo(); }, [](TextComponent &t) { t.redo(); });
  add_separator();

  add_row("Cut", [](TextComponent const &t) { return t.is_editable() and t.has_selection(); }, [](TextComponent &t) { t.cut(); });
  add_row("Copy", [](TextComponent const &t) { return t.has_selection(); }, [](TextComponent &t) { t.copy(); });
  add_row("Paste", [](TextComponent const &t) { return t.is_editable() and Clipboard::has_text(); }, [](TextComponent &t) { t.paste(); });
  add_row("Delete", [](TextComponent const &t) { return t.is_editable(); }, [](TextComponent &t) { t.delete_forward(); });
  add_separator();

  add_row("Select All", [](TextComponent const &) { return true; }, [](TextComponent &t) { t.select_all(); });

  // The rows' enablement is a property of the moment the menu is shown, not
  // of the moment it was built: the selection the Copy row would copy may
  // have been made after the popup was created (a Swing text popup asks its
  // actions on every show the same way). BECOMES_VISIBLE fires before the
  // popup window is shown, so the first paint already has the right rows.
  add_listener([this](PopupMenuEvent &e) {
    if (e.id == PopupMenuEvent::BECOMES_VISIBLE) {
      refresh_actions();
    }
  });
}

void TextPopupMenu::refresh_actions() {
  for (auto &&action : this->actions) {
    action->refresh();
  }
}

std::shared_ptr<TextPopupMenu> create_text_popup_menu(std::shared_ptr<TextComponent> const &target) {
  return make_component<TextPopupMenu>(target);
}

}
