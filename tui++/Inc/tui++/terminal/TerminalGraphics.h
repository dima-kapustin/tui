#pragma once

#include <tui++/Graphics.h>
#include <tui++/terminal/TerminalScreen.h>

namespace tui::terminal {

class TerminalGraphics: public Graphics {
  TerminalScreen &screen;

  Font font;
  int dx, dy;
  Rectangle clip;

  Color foreground_color;
  Color background_color;
  Attributes attributes = Attributes::NONE;

public:
  TerminalGraphics(TerminalScreen &screen) :
      TerminalGraphics(screen, Rectangle { 0, 0, screen.get_width(), screen.get_height() }, 0, 0) {
  }

  TerminalGraphics(TerminalScreen &screen, const Rectangle &clip_rect, int dx, int dy) :
      screen(screen), dx(dx), dy(dy), clip(clip_rect) {
  }

private:
  void reset(const Rectangle &r);

  void update_attributes();

public:
  void clip_rect(int x, int y, int width, int height) override;

  std::unique_ptr<Graphics> create();

  std::unique_ptr<Graphics> create(int x, int y, int width, int height) override;

  void draw_char(Char ch, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  void draw_hline(int x, int y, int length, Char ch, const Attributes &attributes = Attributes::NONE) override;

  void draw_rect(int x, int y, int width, int height) override;

  void draw_string(const std::string &str, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  void draw_vline(int x, int y, int length, Char ch, const Attributes &attributes = Attributes::NONE) override;

  void fill_rect(int x, int y, int width, int height) override;

  Rectangle get_clip_rect() const override;
  void set_clip_rect(const Rectangle &rect) override;

  Color get_foreground_color() const override;
  void set_foreground_color(const Color &color) override;

  Color get_background_color() const override;
  void set_background_color(const Color &color) override;

  Font get_font() const override;
  void set_font(const Font &font) override;

  void translate(int dx, int dy) override;

  void flush() {
    this->screen.flush();
  }
};

}
