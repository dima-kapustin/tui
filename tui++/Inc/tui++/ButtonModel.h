#pragma once

#include <tui++/Char.h>
#include <tui++/Event.h>
#include <tui++/Object.h>

#include <tui++/event/EventSource.h>

#include <memory>

namespace tui {

class MenuItem;
class ButtonGroup;

class ButtonModel: public Object, public EventSource<ChangeEvent, ItemEvent, ActionEvent>, public std::enable_shared_from_this<ButtonModel> {
  std::weak_ptr<ButtonGroup> group;

  Char mnemonic;
  ActionKey action_command;

protected:
  struct {
    unsigned is_selected :1;
    unsigned is_enabled :1;
    unsigned is_pressed :1;
    unsigned is_armed :1;
    unsigned is_rollover :1;
    unsigned is_menu_item :1;
  } state;

public:
  std::shared_ptr<ButtonGroup> get_group() const {
    return this->group.lock();
  }

  void set_group(std::shared_ptr<ButtonGroup> const &group) {
    this->group = group;
  }

  bool is_enabled() const {
    return this->state.is_enabled;
  }

  void set_enabled(bool value) {
    if (value != this->state.is_enabled) {
      this->state.is_enabled = value;
      this->state.is_pressed &= value;
      this->state.is_armed &= value;
      fire_state_changed();
    }
  }

  bool is_selected() const {
    return this->state.is_selected;
  }

  virtual void set_selected(bool value);

  bool is_pressed() const {
    return this->state.is_pressed;
  }

  virtual void set_pressed(bool value);

  bool is_armed() const {
    return this->state.is_armed;
  }

  virtual void set_armed(bool value);

  bool is_rollover() const {
    return this->state.is_rollover;
  }

  void set_rollover(bool value) {
    if (is_rollover() != value and is_enabled()) {
      this->state.is_rollover = value;
      fire_state_changed();
    }
  }

  Char const& get_mnemonic() const {
    return this->mnemonic;
  }

  void set_mnemonic(Char const &mnemonic) {
    this->mnemonic = mnemonic;
    fire_state_changed();
  }

  ActionKey const& get_action_command() const {
    return this->action_command;
  }

  void set_action_command(ActionKey const &action_command) {
    this->action_command = action_command;
  }

private:
  void fire_state_changed() {
    fire_event<ChangeEvent>(shared_from_this());
  }

  bool is_menu_item() const {
    return this->state.is_menu_item;
  }

  void make_menu_item() {
    this->state.is_menu_item = true;
  }
};

}
