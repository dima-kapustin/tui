#pragma once

#include <tui++/Action.h>

namespace tui {

class AbstractAction: public Action {
public:
  virtual bool is_enabled() const override {
    return this->enabled;
  }

  virtual void set_enabled(bool value) override {
    this->enabled = value;
  }

  virtual std::string get_name() const override {
    return this->name;
  }

  virtual void set_name(std::string const &name) override {
    this->name = name;
  }

  virtual std::string get_short_description() const override {
    return this->short_description;
  }

  virtual void set_short_description(std::string const &short_description) override {
    this->short_description = short_description;
  }

  virtual std::string get_long_description() const override {
    return this->long_description;
  }

  virtual void set_long_description(std::string const &long_description) override {
    this->long_description = long_description;
  }

  virtual ActionKey get_action_command() const override {
    return this->action_command;
  }

  virtual void set_action_command(ActionKey const &action_command) override {
    this->action_command = action_command;
  }

  virtual Char get_mnemonic() const override {
    return this->mnemonic;
  }

  virtual void set_mnemonic(Char const &mnemonic) override {
    this->mnemonic = mnemonic;
  }

  virtual KeyStroke get_accelerator() const override {
    return this->accelerator;
  }

  virtual void set_accelerator(KeyStroke const &accelerator) override {
    this->accelerator = accelerator;
  }

  virtual bool is_selected() const override {
    return this->selected;
  }

  virtual void set_selected(bool value) override {
    this->selected = value;
  }

  virtual int get_displayed_mnemonic_index() const override {
    return this->displayed_mnemonic_index;
  }

  virtual void set_displayed_mnemonic_index(int displayed_mnemonic_index) override {
    this->displayed_mnemonic_index = displayed_mnemonic_index;
  }

private:
  Property<bool> enabled { this, "Enabled" };
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
