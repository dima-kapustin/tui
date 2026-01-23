#include <tui++/lookandfeel/border/MarginBorder.h>

#include <tui++/Component.h>
#include <tui++/MarginContainer.h>

namespace tui::laf {

Insets MarginBorder::get_border_insets(const Component &c) const {
  if (auto *margin_container = dynamic_cast<MarginContainer const*>(&c)) {
    if (auto const &margin = margin_container->get_margin()) {
      return *margin;
    }
  }
  return {};
}

}
