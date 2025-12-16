#include <tui++/ButtonGroup.h>

#include <tui++/ButtonModel.h>
#include <tui++/AbstractButton.h>

namespace tui {

void ButtonGroup::add(std::shared_ptr<AbstractButton> const &button) {
  if (button) {
    this->buttons.emplace_back(button);
    if (button->is_selected()) {
      if (this->selection.expired()) {
        this->selection = button->get_model();
      } else {
        button->set_selected(false);
      }
    }
    button->get_model()->set_group(shared_from_this());
  }
}

void ButtonGroup::remove(std::shared_ptr<AbstractButton> const &button) {
  if (button) {
    std::erase_if(this->buttons, [&button](auto &candidate) {
      return candidate == button;
    });
    if (button->get_model() == this->selection.lock()) {
      this->selection.reset();
    }
    button->get_model()->set_group(nullptr);
  }
}

void ButtonGroup::clear_selection() {
  if (auto old_selection = this->selection.lock()) {
    this->selection.reset();
    old_selection->set_selected(false);
  }
}

void ButtonGroup::set_selected(std::shared_ptr<ButtonModel> const &m, bool value) {
  if (value and m and m != this->selection.lock()) {
    auto old_selection = this->selection.lock();
    this->selection = m;
    if (old_selection) {
      old_selection->set_selected(false);
    }
    m->set_selected(true);
  }
}

}
