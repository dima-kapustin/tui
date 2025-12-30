#pragma once

#include <vector>

#include <tui++/Char.h>
#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/Attributes.h>

#include <tui++/util/utf-8.h>

namespace tui {

class Terminal;
class TerminalGraphics;

class TerminalScreen: public Screen {
  using base = Screen;

  struct CharView {
    Char ch = ' ';
    Attributes attributes = Attributes::NONE;
    Color foreground_color = DEFAULT_COLOR;
    Color background_color = DEFAULT_COLOR;

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
  TerminalScreen() noexcept {
    resize_view();
  }

  void terminal_resized();
  void print();

  friend class Terminal;

private:
  void resize_view();

  void draw_char(Char ch, int x, int y, const Color &foreground_color, const Color &background_color, const Attributes &attributes) {
    auto &cv = this->view[y][x];
    cv.ch = ch;
    cv.attributes = attributes;
    cv.foreground_color = foreground_color;
    cv.background_color = background_color;
  }

  friend class TerminalGraphics;

public:
  void move_cursor_to(int line, int column);
  void move_cursor_by(int lines, int columns);

  virtual void run_event_loop() override;
  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const& clip) override;

  virtual void refresh();

  void clear();
  void flush();
};

}

