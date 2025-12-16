#pragma once

#include <memory>
#include <vector>

namespace tui {

class ButtonModel;
class AbstractButton;

class ButtonGroup: public std::enable_shared_from_this<ButtonGroup> {
  std::vector<std::shared_ptr<AbstractButton>> buttons;
  std::weak_ptr<ButtonModel> selection;

public:
  void add(std::shared_ptr<AbstractButton> const &button);
  void remove(std::shared_ptr<AbstractButton> const &button);

  void clear_selection();

  const std::vector<std::shared_ptr<AbstractButton>>& get_buttons() const {
    return this->buttons;
  }

  std::shared_ptr<ButtonModel> get_selection() const {
    return this->selection.lock();
  }

  void set_selected(std::shared_ptr<ButtonModel> const &m, bool value);

  bool is_selected(std::shared_ptr<ButtonModel> const &m) const {
    return m == this->selection.lock();
  }

  int get_button_count() const {
    return this->buttons.size();
  }
};

}
