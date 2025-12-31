#pragma once

#include <tui++/Graphics.h>

namespace tui {

class TerminalScreen;

class TerminalGraphics: public Graphics {
  struct BoxCharacters {
    Char top_left;
    Char top;
    Char top_right;
    Char right;
    Char bottom_right;
    Char bottom;
    Char bottom_left;
    Char left;
    Char horizontal;
    Char vertical;
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
  virtual void clip_rect(int x, int y, int width, int height) override;

  virtual std::unique_ptr<Graphics> create();

  virtual std::unique_ptr<Graphics> create(int x, int y, int width, int height) override;

  virtual void draw_char(const Char &c, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  virtual void draw_hline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) override;

  virtual void draw_rect(int x, int y, int width, int height) override;

  virtual void draw_rounded_rect(int x, int y, int width, int height) override;

  virtual void draw_string(const std::string &str, int x, int y, const Attributes &attributes = Attributes::NONE) override;

  virtual void draw_vline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) override;

  virtual void fill_rect(int x, int y, int width, int height) override;

  virtual Rectangle get_clip_rect() const override;
  virtual void set_clip_rect(const Rectangle &rect) override;

  virtual bool hit_clip_rect(int x, int y, int width, int height) const override;

  virtual Color get_foreground_color() const override;
  virtual void set_foreground_color(const Color &color) override;

  virtual Color get_background_color() const override;
  virtual void set_background_color(const Color &color) override;

  virtual Font get_font() const override;
  virtual void set_font(const Font &font) override;

  virtual Stroke get_stroke() const override;
  virtual void set_stroke(Stroke stroke) override;

  virtual void translate(int dx, int dy) override;

  void flush();
};

}
