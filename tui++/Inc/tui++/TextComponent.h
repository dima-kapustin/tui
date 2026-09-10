#pragma once

// TextComponent - the command surface every text view of the toolkit shares,
// the way Swing's JTextComponent is the common surface of its text family.
// The commands are the ones Swing's DefaultEditorKit names
// ("cut-to-clipboard", "copy-to-clipboard", "paste-from-clipboard",
// "delete-next-char", "select-all", "undo", "redo"), so one implementation --
// the standard text popup menu (see TextPopupMenu) -- can drive any of them,
// as BasicTextUI's popup drives every JTextComponent through its EditorKit.
//
// Both of the toolkit's text views, TextField and TextArea, implement it.
// A program that has to handle "a text component" (a dialog that reads the
// selection, a command that disables itself on a read-only view) can hold a
// std::shared_ptr<TextComponent> instead of knowing which kind it was given.

namespace tui {

class TextComponent {
public:
  virtual ~TextComponent() = default;

  // Whether the component accepts edits (Swing's JTextComponent.isEditable).
  // A read-only view still selects and copies.
  virtual bool is_editable() const = 0;

  // Whether there is a selection to copy or delete.
  virtual bool has_selection() const = 0;

  virtual void select_all() = 0;

  // The edit history. Both stacks are empty on a read-only view, so the
  // Undo/Redo rows of the popup disable themselves without a special case.
  virtual bool can_undo() const = 0;
  virtual bool can_redo() const = 0;
  virtual void undo() = 0;
  virtual void redo() = 0;

  // The clipboard and deletion commands (Swing's DefaultEditorKit actions).
  virtual void cut() = 0;
  virtual void copy() = 0;
  virtual void paste() = 0;

  // Deletes the selection, or the character after the caret when there is
  // none (Swing's delete-next-char, the Delete row of the text popup).
  virtual void delete_forward() = 0;
};

}
