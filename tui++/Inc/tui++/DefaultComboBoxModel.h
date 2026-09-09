#pragma once

#include <tui++/ComboBoxModel.h>

#include <string>
#include <vector>

namespace tui {

// Swing's DefaultComboBoxModel: a mutable, string-backed ComboBoxModel. The
// mutation operations (add/insert/remove/replace) fire a ChangeEvent on
// every structural change; the selection helpers fire one whenever the
// selection moves. The selection survives removals that leave it valid:
// removing an item before the selected one shifts the selection index down,
// removing the selected item itself clears the selection.
class DefaultComboBoxModel: public ComboBoxModel {
  std::vector<std::string> items;
  std::optional<size_t> selected_index;

  void fire_contents_changed() {
    fire_event<ChangeEvent>(shared_from_this());
  }

public:
  DefaultComboBoxModel() = default;

  DefaultComboBoxModel(std::vector<std::string> items) :
      items(std::move(items)) {
  }

  // ---- ComboBoxModel ----

  size_t get_size() const override {
    return this->items.size();
  }

  std::string get_item_at(size_t index) const override {
    return this->items.at(index);
  }

  std::optional<size_t> get_selected_index() const override {
    return this->selected_index;
  }

  void set_selected_index(std::optional<size_t> index) override {
    if (index and *index >= get_size()) {
      index = std::nullopt;
    }
    if (this->selected_index != index) {
      this->selected_index = index;
      fire_contents_changed();
    }
  }

  // ---- MutableComboBoxModel (Swing's MutableComboBoxModel, folded in) ----

  void add_item(std::string const &item) override {
    this->items.push_back(item);
    fire_contents_changed();
  }

  void insert_item_at(std::string const &item, size_t index) override {
    if (index > get_size()) {
      index = get_size();
    }
    this->items.insert(this->items.begin() + index, item);
    if (this->selected_index and *this->selected_index >= index) {
      this->selected_index = *this->selected_index + 1;
    }
    fire_contents_changed();
  }

  void remove_item_at(size_t index) override {
    if (index >= get_size()) {
      return;
    }
    this->items.erase(this->items.begin() + index);
    if (this->selected_index) {
      if (*this->selected_index == index) {
        this->selected_index = std::nullopt;
      } else if (*this->selected_index > index) {
        this->selected_index = *this->selected_index - 1;
      }
    }
    fire_contents_changed();
  }

  void remove_all_items() override {
    if (not this->items.empty()) {
      this->items.clear();
      this->selected_index = std::nullopt;
      fire_contents_changed();
    }
  }

  // Replaces the whole contents; the selection is kept when the selected
  // index is still within the new list, otherwise it is cleared.
  void set_items(std::vector<std::string> items) {
    this->items = std::move(items);
    if (this->selected_index and *this->selected_index >= get_size()) {
      this->selected_index = std::nullopt;
    }
    fire_contents_changed();
  }
};

}
