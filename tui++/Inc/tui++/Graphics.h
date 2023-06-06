#pragma once

#include <tui++/Char.h>
#include <tui++/Font.h>
#include <tui++/Color.h>
#include <tui++/Rectangle.h>
#include <tui++/Attributes.h>

#include <memory>

namespace tui {

class Graphics {
public:
  virtual ~Graphics() {
  }

  virtual void clip_rect(int x, int y, int width, int height) = 0;
  void clip_rect(const Rectangle &rect) {
    clip_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual std::unique_ptr<Graphics> create(int x, int y, int width, int height) = 0;
  std::unique_ptr<Graphics> create(const Rectangle &rect) {
    return create(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_char(Char ch, int x, int y, const Attributes &attributes = Attributes::NONE) = 0;

  /**
   * Draws a horizontal line, using the current color, starting at the given point <code>(x1,&nbsp;y1)</code> in this graphics context's
   * coordinate system, with the given length and character.
   */
  virtual void draw_hline(int x, int y, int length, Char ch, const Attributes &attributes = Attributes::NONE) = 0;
  void draw_hline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) {
    draw_hline(x, y, length, BoxDrawing::HORIZONTAL_LIGHT, attributes);
  }

  virtual void draw_rect(int x, int y, int width, int height) = 0;
  void draw_rect(const Rectangle &rect) {
    draw_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_string(const std::string &str, int x, int y, const Attributes &attributes = Attributes::NONE) = 0;

  /**
   * Draws a vertical line, using the current color, starting at the given point <code>(x1,&nbsp;y1)</code> in this graphics context's
   * coordinate system, with the given length and character.
   */
  virtual void draw_vline(int x, int y, int length, Char ch, const Attributes &attributes = Attributes::NONE) = 0;
  /**
   * Draws a vertical line, using the current color, starting at the given point <code>(x1,&nbsp;y1)</code> in this graphics context's
   * coordinate system and with the given length.
   */
  void draw_vline(int x, int y, int length, const Attributes &attributes = Attributes::NONE) {
    draw_vline(x, y, length, BoxDrawing::VERTICAL_LIGHT, attributes);
  }

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

  virtual void translate(int dx, int dy) = 0;
};

}
