#include <tui++/lookandfeel/Translations.h>

#include <tui++/Component.h>
#include <tui++/Messages.h>

namespace tui::laf {

std::string displayed_text(Component const &component, std::string_view text) {
  if (auto translatable = component.get_client_property<bool>(Messages::TRANSLATABLE_PROPERTY); translatable and not *translatable) {
    return std::string { text };
  }
  return Messages::get(text);
}

}
