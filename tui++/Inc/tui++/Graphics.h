#pragma once

#include <tui++/Font.h>
#include <tui++/Color.h>
#include <tui++/Stroke.h>
#include <tui++/Rectangle.h>
#include <tui++/Attributes.h>

#include <memory>

namespace tui {

class Char;

class Graphics {
public:
  virtual ~Graphics() {
  }

  virtual std::unique_ptr<Graphics> create(int x, int y, int width, int height) = 0;
  std::unique_ptr<Graphics> create(const Rectangle &rect) {
    return create(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void clip_rect(int x, int y, int width, int height) = 0;
  void clip_rect(const Rectangle &rect) {
    clip_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_char(const Char &c, int x, int y, const Attributes &attributes = Attributes::NONE) = 0;

  virtual void draw_hline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) = 0;

  virtual void draw_rect(int x, int y, int width, int height) = 0;
  void draw_rect(const Rectangle &rect) {
    draw_rect(rect.x, rect.y, rect.width, rect.height);
  }
  virtual void draw_rounded_rect(int x, int y, int width, int height) = 0;
  void draw_rounded_rect(const Rectangle &rect) {
    draw_rounded_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_string(const std::string &str, int x, int y, const Attributes &attributes = Attributes::NONE) = 0;

  virtual void draw_vline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) = 0;

  virtual void fill_rect(int x, int y, int width, int height) = 0;
  void fill_rect(const Rectangle &rect) {
    fill_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual Rectangle get_clip_rect() const = 0;
  virtual void set_clip_rect(const Rectangle &rect) = 0;

  virtual Color get_foreground_color() const = 0;
  virtual void set_foreground_color(const Color &color) = 0;

  virtual Color get_background_color() const = 0;
  virtual void set_background_color(const Color &color) = 0;

  virtual Font get_font() const = 0;
  virtual void set_font(const Font &font) = 0;

  virtual Stroke get_stroke() const = 0;
  virtual void set_stroke(Stroke stroke) = 0;

  virtual void translate(int dx, int dy) = 0;
};

}
