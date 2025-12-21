#include <tui++/lookandfeel/PopupMenuUI.h>

#include <tui++/Popup.h>
#include <tui++/PopupMenu.h>

namespace tui::laf {

std::shared_ptr<Popup> PopupMenuUI::get_popup(std::shared_ptr<PopupMenu> const &menu, int x, int y) {
  return std::make_shared<Popup>(menu->get_invoker(), menu, x, y);
}

}
