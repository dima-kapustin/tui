#pragma once

#include <vector>
#include <optional>

#include <tui++/Char.h>
#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/Attributes.h>

#include <tui++/terminal/text/TextColor.h>

namespace tui {

class Terminal;
class TextGraphics;

class TextScreen: public Screen {
  using base = Screen;

  struct CharView {
    Char ch = ' ';
    Attributes attributes = Attributes::NONE;
    TextColor foreground_color = { };
    TextColor background_color = { };
  };

  static CharView EMPTY_CHAR_VIEW;

  std::vector</* rows */std::vector</* columns */CharView>> view;

  std::shared_ptr<laf::LookAndFeel> look_and_feel;
  std::shared_ptr<TextMetrics> text_metrics;

private:
  TextScreen() noexcept;

  void print();
  void print_rows(int first_row, int last_row);

  // Emits only the rows a damaged region touches, and within each row either
  // the whole row or just the damaged column span (see the heuristic in
  // print_rows_region): rows outside the region keep their last emitted
  // content and are skipped entirely.
  void print_rows_region(Rectangle const &region);

  friend class Terminal;

private:
  void resize_view();

  TextColor to_terminal(Color const& c);

  void draw_char(Char ch, int x, int y, std::optional<Color> const &foreground_color, std::optional<Color> const &background_color, std::optional<Attributes> const &attributes) {
    auto &cv = this->view[y][x];
    cv.ch = ch;
    if (attributes) {
      cv.attributes = attributes.value();
    }
    if (foreground_color) {
      cv.foreground_color = to_terminal(foreground_color.value());
    }
    if (background_color) {
      cv.background_color = to_terminal(background_color.value());
    }
  }

  friend class TextGraphics;

public:
  void move_cursor_to(int line, int column);
  void move_cursor_by(int lines, int columns);

  virtual void run_event_loop() override;
  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const &clip) override;

  virtual std::shared_ptr<laf::LookAndFeel> get_look_and_feel() const override {
    return this->look_and_feel;
  }

  virtual std::shared_ptr<TextMetrics> get_text_metrics() const override {
    return this->text_metrics;
  }

  virtual void refresh();

  // Repaints only `rect` (screen coordinates): paints the component tree
  // with a graphics clipped to the region and flushes the view, so edits
  // touch just the damaged cells.
  virtual void repaint_region(Rectangle const &rect) override;

  virtual void resized() override;

  void clear();
  void flush();
};

}

