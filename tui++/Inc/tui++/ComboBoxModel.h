#pragma once

#include <tui++/Object.h>

#include <tui++/event/ChangeEvent.h>
#include <tui++/event/EventSource.h>

#include <memory>
#include <optional>
#include <string>

namespace tui {

// Swing's ComboBoxModel: the items and the selection behind a ComboBox. The
// model is the single source of truth for both; a ChangeEvent announces any
// contents or selection change, and the ComboBox listens to its model so a
// replacement model (set_model) drives the component the same way the
// default one does.
//
// The selection is index based (Swing's ComboBoxModel keeps the selected
// *item*; the index is equivalent as long as items are identified by their
// text, which the TUI has to render anyway). No selection -- an editable
// combo whose field holds text that matches no item -- is `std::nullopt`.
class ComboBoxModel: public Object, public EventSource<ChangeEvent>, public std::enable_shared_from_this<ComboBoxModel> {
public:
  virtual ~ComboBoxModel() = default;

  virtual size_t get_size() const = 0;

  virtual std::string get_item_at(size_t index) const = 0;

  virtual std::optional<size_t> get_selected_index() const = 0;

  virtual void set_selected_index(std::optional<size_t> index) = 0;

  // ---- mutations. The default implementations do nothing: a custom model
  // implements only what its items support (a DefaultComboBoxModel
  // implements all of them). The ComboBox mutation helpers degrade
  // gracefully on a read-only model.

  virtual void add_item(std::string const &item) {
  }

  virtual void insert_item_at(std::string const &item, size_t index) {
  }

  virtual void remove_item_at(size_t index) {
  }

  virtual void remove_all_items() {
  }
};

}
