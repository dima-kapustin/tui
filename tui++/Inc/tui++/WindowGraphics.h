#pragma once

#include <tui++/Graphics.h>

namespace tui {

class WindowGraphics: public Graphics {
  Color foreground_color;
  Color background_color;
  Font font;
  int attributes = 0;
  Rectangle clip;
  int dx, dy;

private:
  WindowGraphics() {
    WindowGraphics( { }, 0, 0);
  }

  WindowGraphics(const Rectangle &clip_rect, int dx, int dy) :
      clip(clip_rect), dx(dx), dy(dy) {
  }

private:
  void reset(const Rectangle &r);

  void update_attributes();

public:
  void clip_rect(int x, int y, int width, int height) override;

  std::unique_ptr<Graphics> create();

  std::unique_ptr<Graphics> create(int x, int y, int width, int height) override;

  void draw_char(int ch, int x, int y, int attributes) override;

  void draw_hline(int x, int y, int length, const std::string &chr) override;

  void draw_rect(int x, int y, int width, int height) override;

  void draw_string(const std::string &str, int x, int y, int attributes) override;

  void draw_vline(int x, int y, int length, const std::string &chr) override;

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
};

}
