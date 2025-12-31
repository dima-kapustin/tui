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
  virtual ~Action() {
  }

public:
  virtual bool accept(const std::shared_ptr<Object> &sender) const {
    return is_enabled();
  }

  virtual bool is_enabled() const = 0;
  virtual void set_enabled(bool value) = 0;

  virtual std::string get_name() const {
    return get_property_value<std::string>(NAME);
  }
  virtual void set_name(std::string const &name) {
    set_property_value(NAME, name);
  }

  virtual std::string get_short_description() const {
    return get_property_value<std::string>(SHORT_DESCRIPTION);
  }
  virtual void set_short_description(std::string const &short_description) {
    set_property_value(SHORT_DESCRIPTION, short_description);
  }

  virtual std::string get_long_description() const {
    return get_property_value<std::string>(LONG_DESCRIPTION);
  }
  virtual void set_long_description(std::string const &long_description) {
    set_property_value(LONG_DESCRIPTION, long_description);
  }

  virtual ActionKey get_action_command() const {
    return get_property_value<ActionKey>(ACTION_COMMAND);
  }

  virtual void set_action_command(ActionKey const &action_command) {
    set_property_value(ACTION_COMMAND, action_command);
  }

  virtual Char get_mnemonic() const {
    return get_property_value<Char>(MNEMONIC);
  }

  virtual void set_mnemonic(Char const &mnemonic) {
    set_property_value<Char>(MNEMONIC, mnemonic);
  }

  virtual KeyStroke get_accelerator() const {
    return get_property_value<KeyStroke>(ACCELERATOR);
  }

  virtual void set_accelerator(KeyStroke const &accelerator) {
    set_property_value(ACCELERATOR, accelerator);
  }

  virtual bool is_selected() const {
    return get_property_value<bool>(SELECTED);
  }

  virtual void set_selected(bool value) {
    set_property_value(SELECTED, value);
  }

  virtual int get_displayed_mnemonic_index() const {
    return get_property_value<int>(DISPLAYED_MNEMONIC_INDEX);
  }

  virtual void set_displayed_mnemonic_index(int displayed_mnemonic_index) {
    set_property_value(DISPLAYED_MNEMONIC_INDEX, displayed_mnemonic_index);
  }
};

}
