#include <tui++/RadioButtonMenuItem.h>

#include <tui++/ToggleButtonModel.h>

namespace tui {

void RadioButtonMenuItem::init() {
  base::init();
  // As in Swing's JRadioButtonMenuItem: the item keeps its state across
  // picks, so its model is the toggle model (a ButtonGroup then arbitrates
  // which member of a group stays selected). The mnemonic was stored on the
  // plain ButtonModel the MenuItem constructor created; re-apply it to the
  // new model.
  set_model(std::make_shared<ToggleButtonModel>());
  set_mnemonic(get_mnemonic());
}

}
