#include <tui++/Action.h>

#include <cassert>
#include <iostream>

using namespace tui;

class TestAction: public Action {
  Property<bool> enabled { this, "enabled", false };
public:
  virtual bool is_enabled() const override {
    return this->enabled;
  }

  virtual void set_enabled(bool value) override {
    this->enabled = value;
  }

  virtual void action_performed(ActionEvent &e) {

  }
};

constexpr auto NAME = "TestAction";
constexpr auto SHORT_DESCRIPTION = "New Test Action";
constexpr auto MNEMONIC = 'T';

void test_Action() {
  auto action = TestAction { };
  action.add_property_change_listener([](PropertyChangeEvent &e) {
    if (e.property_name == Action::NAME) {
      assert(std::any_cast<std::string>(e.old_value) == "");
      assert(std::any_cast<std::string>(e.new_value) == NAME);
    } else if (e.property_name == Action::SHORT_DESCRIPTION) {
      assert(std::any_cast<std::string>(e.old_value) == "");
      assert(std::any_cast<std::string>(e.new_value) == SHORT_DESCRIPTION);
    } else if (e.property_name == Action::MNEMONIC) {
      assert(std::any_cast<Char>(e.old_value) == '\0');
      assert(std::any_cast<Char>(e.new_value) == MNEMONIC);
    } else if (e.property_name == "enabled") {
      assert(std::any_cast<bool>(e.old_value) == false);
      assert(std::any_cast<bool>(e.new_value) == true);
    }
  });

  action.set_name(NAME);
  action.set_short_description(SHORT_DESCRIPTION);
  action.set_mnemonic(MNEMONIC);
  action.set_enabled(true);

  assert(action.get_name() == NAME);
  assert(action.get_short_description() == SHORT_DESCRIPTION);
  assert(action.get_mnemonic() == MNEMONIC);
}
