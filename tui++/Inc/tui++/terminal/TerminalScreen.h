#pragma once

#include <vector>

#include <tui++/Char.h>
#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/Attributes.h>

#include <tui++/util/utf-8.h>

namespace tui::terminal {

class Terminal;
class TerminalGraphics;

class TerminalScreen: public Screen {
  using base = Screen;

  Terminal &terminal;

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
  TerminalScreen(Terminal &terminal) noexcept :
      terminal(terminal) {
    resize_view();
  }

  void terminal_resized();

  friend class Terminal;

private:
  void draw_char(Char ch, int x, int y, const Color &foreground_color, const Color &background_color, const Attributes &attributes) {
    auto &char_view = this->view[y][x];
    char_view.ch = ch;
    char_view.attributes = attributes;
    char_view.foreground_color = foreground_color;
    char_view.background_color = background_color;
  }

  void resize_view();

  friend class TerminalGraphics;

public:
  virtual void run_event_loop() override;

  virtual void refresh();

  std::string to_string() const;
  void clear();
  void flush() const;
};

}

