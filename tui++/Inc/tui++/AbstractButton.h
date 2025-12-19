#pragma once

#include <tui++/Event.h>
#include <tui++/Action.h>
#include <tui++/Component.h>
#include <tui++/ButtonModel.h>

#include <tui++/VerticalAlignment.h>
#include <tui++/HorizontalAlignment.h>
#include <tui++/VerticalTextPosition.h>
#include <tui++/HorizontalTextPosition.h>

namespace tui {

class AbstractButton: public ComponentExtension<Component, ChangeEvent, ItemEvent, ActionEvent> {
  using base = ComponentExtension<Component, ChangeEvent, ItemEvent, ActionEvent>;

protected:
  Property<std::shared_ptr<Action>> action { this, "action" };
  Property<std::shared_ptr<ButtonModel>> model { this, "model" };
  Property<std::string> text { this, "text" };
  Property<bool> hide_action_text { this, "hide-action-text" };
  Property<int> displayed_mnemonic_index { this, "displayed-mnemonic-index", -1 };
  Property<bool> focus_painted { this, "focus-painted", true };
  Property<bool> border_painted { this, "border-painted", true };

  Property<VerticalAlignment> vertical_alignment { this, "vertical-alignment", VerticalAlignment::CENTER };
  Property<HorizontalAlignment> horizontal_alignment { this, "horizontal-alignment", HorizontalAlignment::CENTER };

  Property<VerticalTextPosition> vertical_text_position { this, "vertical-text-position", VerticalTextPosition::CENTER };
  Property<HorizontalTextPosition> horizontal_text_position { this, "horizontal-text-position", HorizontalTextPosition::TRAILING };

  ChangeListener change_listener = std::bind(state_changed, this, std::placeholders::_1);
  ItemListener item_listener = std::bind(item_state_changed, this, std::placeholders::_1);
  ActionListener action_listener = std::bind(action_performed, this, std::placeholders::_1);
  PropertyChangeListener action_property_change_listener = std::bind(action_property_changed, this, std::placeholders::_1);

  Char mnemonic;

  struct {
    unsigned is_border_painted_set :1;
    unsigned is_rollover_enabled_set :1;
    unsigned is_icon_text_gap_set :1;
    unsigned is_content_area_filled_set :1;
  } flags { };

protected:
  AbstractButton(std::string const &text = "") {
    this->text = text;
  }

  void init() override;

  using base::fire_event;

  virtual void state_changed(ChangeEvent &e);
  virtual void item_state_changed(ItemEvent &e);
  virtual void action_performed(ActionEvent &e);
  virtual void action_property_changed(PropertyChangeEvent &e);

  virtual bool should_update_selected_state_from_action() const {
    return false;
  }

  void add_impl(const std::shared_ptr<Component> &c, const std::any &constraints, int z_order) override;

  virtual void configure_properties_from_action();

public:
  std::shared_ptr<ButtonModel> get_model() const {
    return this->model;
  }

  virtual void set_model(std::shared_ptr<ButtonModel> const &model);

  void set_enabled(bool value) override {
    if (not value and this->model->is_rollover()) {
      this->model->set_rollover(false);
    }
    base::set_enabled(value);
    this->model->set_enabled(value);
  }

  std::string const& get_text() const {
    return this->text;
  }

  void set_text(std::string const &text) {
    auto old_text = this->text.value();
    this->text = text;

    if (old_text != text) {
      revalidate();
      repaint();
    }
  }

  ActionKey const& get_action_command() const {
    if (auto &action_command = this->model->get_action_command(); not action_command.empty()) {
      return action_command;
    }
    return get_text();
  }

  void set_action_command(ActionKey const &action_command) {
    this->model->set_action_command(action_command);
  }

  bool is_selected() const {
    return this->model->is_selected();
  }

  void set_selected(bool value) {
    this->model->set_selected(value);
  }

  bool get_hide_action_text() const {
    return this->hide_action_text;
  }

  void set_hide_action_text(bool hide_action_text) {
    if (this->hide_action_text != hide_action_text) {
      set_text(this->action and not hide_action_text ? this->action->get_name() : "");
      this->hide_action_text == hide_action_text;
    }
  }

  int get_displayed_mnemonic_index() const {
    return this->displayed_mnemonic_index;
  }

  void set_displayed_mnemonic_index(int index);

  std::shared_ptr<Action> get_action() const {
    return this->action;
  }

  void set_action(const std::shared_ptr<Action> &action);

  Char const& get_mnemonic() const {
    return this->mnemonic;
  }

  void set_mnemonic(Char const &mnemonic);

  bool is_focus_painted() const {
    return this->focus_painted;
  }

  void set_focus_painted(bool value) {
    if (this->focus_painted != value) {
      this->focus_painted = value;
      revalidate();
      repaint();
    }
  }

  bool is_border_painted() const {
    return this->border_painted;
  }

  void set_border_painted(bool value) {
    if (this->border_painted != value) {
      this->border_painted = value;
      revalidate();
      repaint();
    }
  }

  VerticalAlignment get_vertical_alignment() const {
    return this->vertical_alignment;
  }

  void set_vertical_alignment(VerticalAlignment value) {
    if (this->vertical_alignment != value) {
      this->vertical_alignment = value;
      repaint();
    }
  }

  HorizontalAlignment get_horizontal_alignment() const {
    return this->horizontal_alignment;
  }

  void set_horizontal_alignment(HorizontalAlignment value) {
    if (this->horizontal_alignment != value) {
      this->horizontal_alignment = value;
      repaint();
    }
  }

  VerticalTextPosition get_vertical_text_position() const {
    return this->vertical_text_position;
  }

  void set_vertical_text_position(VerticalTextPosition value) {
    if (this->vertical_text_position != value) {
      this->vertical_text_position = value;
      revalidate();
      repaint();
    }
  }

  HorizontalTextPosition get_horizontal_text_position() const {
    return this->horizontal_text_position;
  }

  void set_horizontal_text_position(HorizontalTextPosition value) {
    if (this->horizontal_text_position != value) {
      this->horizontal_text_position = value;
      revalidate();
      repaint();
    }
  }

private:
  void update_mnemonic_properties();
  void update_displayed_mnemonic_index(std::string const &text, Char const &mnemonic);

  void set_enabled_from_action();
  void set_selected_from_action();
  void set_displayed_mnemonic_index_from_action();
};

}

