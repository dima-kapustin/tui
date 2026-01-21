#pragma once

#include <vector>
#include <optional>

#include <tui++/Char.h>
#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/Attributes.h>

#include <tui++/terminal/TerminalColor.h>

#include <tui++/util/utf-8.h>

namespace tui {

class Terminal;
class TerminalGraphics;

class TerminalScreen: public Screen {
  using base = Screen;

  struct CharView {
    Char ch = ' ';
    Attributes attributes = Attributes::NONE;
    TerminalColor foreground_color = { };
    TerminalColor background_color = { };

    const size_t get_width() const {
      return util::glyph_width(this->ch);
    }

    const bool is_wide() const {
      return get_width() == 2;
    }
  };

  static CharView EMPTY_CHAR_VIEW;

  std::vector</* rows */std::vector</* columns */CharView>> view;

private:
  TerminalScreen() noexcept;

  void terminal_resized();
  void print();

  friend class Terminal;

private:
  void resize_view();

  TerminalColor to_terminal(Color const& c);

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

  friend class TerminalGraphics;

public:
  void move_cursor_to(int line, int column);
  void move_cursor_by(int lines, int columns);

  virtual void run_event_loop() override;
  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const &clip) override;

  virtual void refresh();

  void clear();
  void flush();
};

}

