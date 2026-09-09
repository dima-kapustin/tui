#include <tui++/CheckBoxMenuItem.h>

#include <tui++/ToggleButtonModel.h>

namespace tui {

void CheckBoxMenuItem::init() {
  base::init();
  // As in Swing's JCheckBoxMenuItem: the item keeps its state across picks,
  // so its model is the toggle model. The mnemonic was stored on the plain
  // ButtonModel the MenuItem constructor created; re-apply it (and the
  // displayed index) to the new model.
  set_model(std::make_shared<ToggleButtonModel>());
  set_mnemonic(get_mnemonic());
}

}
