#pragma once

#include <tui++/Object.h>
#include <tui++/KeyStroke.h>

#include <tui++/event/ActionEvent.h>
#include <tui++/event/EventListener.h>

namespace tui {

class Action: public Object, public EventListener<ActionEvent> {
public:
  constexpr static auto NAME = "Name";
  constexpr static auto SHORT_DESCRIPTION = "ShortDescription";
  constexpr static auto LONG_DESCRIPTION = "LongDescription";
  constexpr static auto ACTION_COMMAND = "ActionCommand";
  constexpr static auto MNEMONIC = "Mnemonic";
  constexpr static auto ACCELERATOR = "Accelerator";
  constexpr static auto SELECTED = "Selected";
  constexpr static auto DISPLAYED_MNEMONIC_INDEX = "DisplayedMnemonicIndex";

public:
  bool accept(const std::shared_ptr<Object> &sender) const {
    return is_enabled();
  }

  bool is_enabled() const {
    return this->enabled;
  }

  void set_enabled(bool value) {
    this->enabled = value;
  }

  std::string const& get_name() const {
    return this->name;
  }

  void set_name(std::string const &name) {
    this->name = name;
  }

  std::string const& get_short_description() const {
    return this->short_description;
  }

  void set_short_description(std::string const &short_description) {
    this->short_description = short_description;
  }

  std::string const& get_long_description() const {
    return this->long_description;
  }

  void set_long_description(std::string const &long_description) {
    this->long_description = long_description;
  }

  ActionKey const& get_action_command() const {
    return this->action_command;
  }

  void set_action_command(ActionKey const &action_command) {
    this->action_command = action_command;
  }

  Char const& get_mnemonic() const {
    return this->mnemonic;
  }

  void set_mnemonic(Char const &mnemonic) {
    this->mnemonic = mnemonic;
  }

  KeyStroke const& get_accelerator() const {
    return this->accelerator;
  }

  void set_accelerator(KeyStroke const &accelerator) {
    this->accelerator = accelerator;
  }

  bool is_selected() const {
    return this->selected;
  }

  void set_selected(bool value) {
    this->selected = value;
  }

  int get_displayed_mnemonic_index() const {
    return this->displayed_mnemonic_index;
  }

  void set_displayed_mnemonic_index(int displayed_mnemonic_index) {
    this->displayed_mnemonic_index = displayed_mnemonic_index;
  }

private:
  Property<bool> enabled { this, "enabled" };
  Property<std::string> name { this, NAME };
  Property<std::string> short_description { this, SHORT_DESCRIPTION };
  Property<std::string> long_description { this, LONG_DESCRIPTION };
  Property<ActionKey> action_command { this, ACTION_COMMAND };
  Property<Char> mnemonic { this, MNEMONIC };
  Property<KeyStroke> accelerator { this, ACCELERATOR };
  Property<bool> selected { this, SELECTED };
  Property<int> displayed_mnemonic_index { this, DISPLAYED_MNEMONIC_INDEX, -1 };
};

}
