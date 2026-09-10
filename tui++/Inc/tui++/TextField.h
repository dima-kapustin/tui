#pragma once

// TextField - Swing's JTextField: a single-line text component with a caret, a
// selection and the classic navigation, editing and clipboard commands.
//
// The component follows the Swing text design: the content, the caret and the
// selection live in the component, and the caret has an independent mark (the
// two ends of the selection, Swing's Caret dot and mark), so every navigation
// command has a selection-extending Shift variant and every edit replaces the
// selection. The command set is the DefaultEditorKit one (the same action
// names, dispatched by the key handler of the same shape):
//   * navigation: Left/Right, Ctrl+Left/Right (words), Home/End; Shift extends
//   * editing: typing, Backspace/Delete, Ctrl+Backspace/Delete (words)
//   * selection: Shift+arrows / Shift+Home/End / Shift+Ctrl+arrows, Ctrl+A,
//     mouse drag, double-click (word) and triple-click (the whole line)
//   * clipboard: Ctrl+X / Ctrl+C / Ctrl+V, plus the classic Windows chords
//     Shift+Delete, Ctrl+Insert and Shift+Insert (the console claims Ctrl+C,
//     so the alternatives matter)
//   * undo/redo: Ctrl+Z, Ctrl+Y
//   * Enter fires an ActionEvent (Swing's postActionEvent)
//
// The field listens on its owning window for keys and takes them while it is
// the focus owner (the framework dispatches keys to windows, not to the focus
// owner). A host widget that keeps the focus while the field edits on its
// behalf -- an editable combo box, as Swing's BasicComboBoxEditor -- drives
// set_editing_focus() and forwards its keys to the key handlers instead.
//
// The content is stored as UTF-32 (the code-point unit), so the caret, the
// selection and the commands count characters, not bytes; UTF-8 is the
// interchange form with the application and the clipboard.

#include <tui++/Component.h>
#include <tui++/Graphics.h>
#include <tui++/TextComponent.h>
#include <tui++/Timer.h>

#include <tui++/event/ActionEvent.h>
#include <tui++/event/ChangeEvent.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace tui {

class ComboBox;

// The field receives keys through this forwarder (see TextField.cpp): it is
// registered on the field's owning window and forwards to the field while the
// field is the keyboard focus owner.
class TextFieldKeyForwarder;

// Receives the drags that follow a press on the field (see TextField.cpp): a
// Component does not process MOUSE_DRAG events itself, the window dispatcher
// retargets them to the press target for screen listeners only.
class TextFieldDragObserver;

class TextField: public ComponentExtension<Component, ActionEvent, ChangeEvent>, public TextComponent {
  using base = ComponentExtension<Component, ActionEvent, ChangeEvent>;

public:
  // Swing's JTextField.HORIZONTAL_ALIGNMENT_* constants: where the text sits
  // while it is narrower than the field.
  enum HorizontalAlignment {
    LEADING = 0,
    CENTER = 1,
    TRAILING = 2
  };

  // One undoable step: the whole state of a single-line field is a snapshot
  // (the text and the two caret ends), so undo restores the caret and the
  // selection with the text, exactly as they were before the edit.
  struct Snapshot {
    std::u32string text;
    std::size_t dot = 0;
    std::size_t mark = 0;
  };

private:
  std::u32string text;

  // The caret (Swing's Caret dot) and the selection's other end (the mark).
  // The selection is the range between them; they are equal when there is
  // none. Both are code-point indices.
  std::size_t dot = 0;
  std::size_t mark = 0;

  // Swing's columns: the preferred width in cells (0 sizes to the content).
  int columns = 0;

  bool editable = true;
  HorizontalAlignment horizontal_alignment = LEADING;

  bool caret_visible = true;
  bool caret_on = true;
  std::chrono::milliseconds caret_blink_rate { 530 };
  Timer blink_timer { std::chrono::milliseconds(530), [this] {
                        blink_tick();
                      } };

  // The host widget (an editable combo box) holds the keyboard focus while
  // this field edits on its behalf; see set_editing_focus.
  bool editing_focus = false;

  // Whether a press began a drag selection (the drags themselves arrive
  // through the screen listener registered for their duration).
  bool mouse_dragging = false;

  std::vector<Snapshot> undo_stack;
  std::vector<Snapshot> redo_stack;

  std::shared_ptr<TextFieldKeyForwarder> key_forwarder;
  std::shared_ptr<TextFieldDragObserver> drag_observer;

  friend class TextFieldKeyForwarder;
  friend class TextFieldDragObserver;

  // The combo box (Swing's BasicComboBoxEditor) forwards its keys here while
  // it owns the focus.
  friend class ComboBox;

public:
  // ---- content (Swing's getText / setText) ----

  std::string get_text() const;

  // Replaces the content; the caret lands after the new text, with no
  // selection (a host that wants the type-to-replace gesture selects it
  // explicitly). The undo history is cleared, as Swing's setText is not an
  // undoable edit.
  void set_text(std::string const &text);

  // ---- sizing and look (Swing's setColumns / setEditable / alignment) ----

  int get_columns() const {
    return this->columns;
  }

  // The preferred width in cells; 0 asks the field to size to its content.
  void set_columns(int columns);

  bool is_editable() const override {
    return this->editable;
  }

  // Whether the field accepts typing and edits (Swing's setEditable). A
  // read-only field still shows its content, its caret and its selection.
  void set_editable(bool value);

  HorizontalAlignment get_horizontal_alignment() const {
    return this->horizontal_alignment;
  }

  void set_horizontal_alignment(HorizontalAlignment alignment);

  // ---- caret and selection (Swing's Caret: dot and mark) ----

  std::size_t get_caret_position() const {
    return this->dot;
  }

  // Moves the caret and clears the selection (Swing's setCaretPosition).
  void set_caret_position(std::size_t position);

  // Moves the caret, leaving the selection's other end where it was (Swing's
  // moveCaretPosition: the Shift+arrow commands).
  void move_caret_position(std::size_t position);

  std::size_t get_selection_start() const;
  std::size_t get_selection_end() const;

  void select(std::size_t start, std::size_t end);

  // Replaces the selection (or inserts at the caret when there is none) with
  // `text`; Swing's replaceSelection. The inserted text is one undoable step.
  void replace_selection(std::string const &text);

  // ---- caret visibility (Swing's Caret visibility and blink rate) ----

  bool is_caret_visible() const {
    return this->caret_visible;
  }

  void set_caret_visible(bool value);

  std::chrono::milliseconds get_caret_blink_rate() const {
    return this->caret_blink_rate;
  }

  // Zero stops the blinking and keeps the caret solid while focused.
  void set_caret_blink_rate(std::chrono::milliseconds rate);

  // ---- editing commands (the DefaultEditorKit actions) ----

  void copy() override;
  void cut() override;
  void paste() override;

  // Deletes the selection, or the character after the caret when there is
  // none (Swing's delete-next-char). The word-wise variant belongs to the
  // editing keys only.
  void delete_forward() override;

  bool has_selection() const override {
    return this->dot != this->mark;
  }

  void undo() override;
  void redo() override;

  bool can_undo() const override {
    return not this->undo_stack.empty();
  }

  bool can_redo() const override {
    return not this->redo_stack.empty();
  }

  void select_all() override;

  // The keyboard's popup trigger opens the standard menu at the caret, as
  // Swing shows a text component's popup at its caret cell.
  virtual Point get_popup_menu_location() const override;

  // Fires the field's ActionEvent (Swing's JTextField.postActionEvent: the
  // Enter key, and the programmatic equivalent).
  void post_action_event();

  void request_input_focus();

  // ---- host integration ----

  // An editable combo box keeps the keyboard focus and forwards its keys to
  // this field; the field then shows and blinks its caret while the host is
  // focused, exactly as if it owned the focus (Swing's editor works the same
  // way).
  void set_editing_focus(bool value);

  bool is_editing_focus() const {
    return this->editing_focus;
  }

  virtual ~TextField();

protected:
  TextField();
  TextField(std::string const &text);
  TextField(int columns);
  TextField(std::string const &text, int columns);

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  virtual void init() override;
  virtual void add_notify() override;
  virtual void remove_notify() override;

  virtual void paint(Graphics &g) override;

private:
  // ---- content geometry (cells; the field scrolls horizontally) ----

  // The text origin and available width, after the border insets and the
  // "TextField.margin" theme insets (the content area).
  int content_left() const;
  int content_width() const;

  // The cell width of the code points before `index` (the caret's cell).
  int cells_before(std::size_t index) const;

  // The first content cell shown: the caret is kept visible by scrolling the
  // content under the field (Swing's DefaultCaret view position), as long as
  // the text does not fit.
  int view_offset() const;

  // The extra x of a text narrower than the field (horizontal alignment).
  int alignment_offset() const;

  // The code-point index nearest to the cell `x` (local coordinates): the
  // mouse commands' hit test.
  std::size_t index_at(int x) const;

  // ---- word boundaries (the word-wise commands and double-click) ----

  static bool is_word_char(char32_t code);
  std::size_t previous_word_boundary(std::size_t from) const;
  std::size_t next_word_boundary(std::size_t from) const;

  // Selects the word (or the run of separators) the clicked spot belongs to:
  // the double-click gesture.
  void select_word_at(std::size_t index);

  // ---- edits ----

  // Replaces [start, end) with `inserted`, one undoable step; the caret lands
  // after the insertion and the selection is cleared.
  void replace_range(std::size_t start, std::size_t end, std::u32string const &inserted);

  void insert_typed(char32_t code);
  void delete_backward(bool word); // Backspace: the selection, or the previous char/word
  void delete_forward(bool word);  // Delete: the selection, or the next char/word

  void push_undo();
  void restore(Snapshot const &snapshot);

  // ---- caret ----

  // Moves the caret to `position` (clamped); `extend` keeps the selection's
  // other end (the Shift variants).
  void move_dot(std::size_t position, bool extend);

  // The plain and Shift arrow commands: an unshifted arrow collapses an
  // existing selection to its near edge first (the classic behavior), then
  // steps one character -- or one word with Ctrl.
  void move_dot_arrow(int direction, bool extend, bool word);
  void move_dot_home(bool extend);
  void move_dot_end(bool extend);

  // ---- state ----

  void notify_change();
  void update_preferred_size();

  bool is_caret_showing() const;
  void restart_caret_blink();
  void blink_tick();
  void focus_changed(bool gained);

  bool is_focused_owner() const;

  // ---- input ----

  void on_key_pressed(KeyEvent &e);
  void on_key_typed(KeyEvent &e);

  void on_mouse_pressed(MousePressEvent &e);
  void on_mouse_click(MouseClickEvent &e);
  void on_mouse_drag(int x);
  void on_mouse_release();
  void register_drag_observer();
  void unregister_drag_observer();
};

}
