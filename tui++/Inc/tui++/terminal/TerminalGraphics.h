#pragma once

#include <tui++/Graphics.h>

namespace tui::terminal {

class TerminalScreen;

class TerminalGraphics: public Graphics {
  struct BoxCharacters {
    const Char &top_left;
    const Char &top;
    const Char &top_right;
    const Char &right;
    const Char &bottom_right;
    const Char &bottom;
    const Char &bottom_left;
    const Char &left;
    const Char &horizontal;
    const Char &vertical;
  };

  static BoxCharacters LIGHT_BOX;
  static BoxCharacters HEAVY_BOX;
  static BoxCharacters DOUBLE_BOX;
  static BoxCharacters ROUNDED_LIGHT_BOX;

  static const BoxCharacters& get_box_chars(Stroke stroke);

private:
  TerminalScreen &screen;

  Font font;
  int dx, dy;
  Rectangle clip;

  Stroke stroke = Stroke::LIGHT;

  Color foreground_color;
  Color background_color;
  Attributes attributes = Attributes::NONE;

public:
  TerminalGraphics(TerminalScreen &screen);
  TerminalGraphics(TerminalScreen &screen, const Rectangle &clip_rect, int dx, int dy);

private:
  void reset(const Rectangle &r);

  void update_attributes();

  void draw_rect(int x, int y, int width, int height, const BoxCharacters &chars);

public:
  void clip_rect(int x, int y, int width, int height) override;

  std::unique_ptr<Graphics> create();

  std::unique_ptr<Graphics> create(int x, int y, int width, int height) override;

  void draw_char(const Char &c, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  void draw_hline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) override;

  void draw_rect(int x, int y, int width, int height) override;

  void draw_rounded_rect(int x, int y, int width, int height) override;

  void draw_string(const std::string &str, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  void draw_vline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) override;

  void fill_rect(int x, int y, int width, int height) override;

  Rectangle get_clip_rect() const override;
  void set_clip_rect(const Rectangle &rect) override;

  Color get_foreground_color() const override;
  void set_foreground_color(const Color &color) override;

  Color get_background_color() const override;
  void set_background_color(const Color &color) override;

  Font get_font() const override;
  void set_font(const Font &font) override;

  Stroke get_stroke() const override;
  void set_stroke(Stroke stroke) override;

  void translate(int dx, int dy) override;

  void flush();
};

}
