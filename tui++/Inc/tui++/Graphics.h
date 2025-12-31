#pragma once

#include <tui++/Font.h>
#include <tui++/Color.h>
#include <tui++/Stroke.h>
#include <tui++/Rectangle.h>
#include <tui++/Attributes.h>

#include <memory>
#include <optional>

namespace tui {

class Char;

class Graphics {
public:
  virtual ~Graphics() {
  }

  virtual std::unique_ptr<Graphics> create(int x, int y, int width, int height) = 0;
  std::unique_ptr<Graphics> create(Rectangle const &rect) {
    return create(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void clip_rect(int x, int y, int width, int height) = 0;
  void clip_rect(Rectangle const &rect) {
    clip_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_char(Char const &c, int x, int y, Attributes const &attributes = Attributes::NONE) = 0;

  virtual void draw_hline(int x, int y, int length, Attributes const &attributes = Attributes::NONE) = 0;

  virtual void draw_rect(int x, int y, int width, int height) = 0;
  void draw_rect(Rectangle const &rect) {
    draw_rect(rect.x, rect.y, rect.width, rect.height);
  }
  virtual void draw_rounded_rect(int x, int y, int width, int height) = 0;
  void draw_rounded_rect(Rectangle const &rect) {
    draw_rounded_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void draw_string(std::string const &str, int x, int y, Attributes const &attributes = Attributes::NONE) = 0;

  virtual void draw_vline(int x, int y, int length, Attributes const &attributes = Attributes::NONE) = 0;

  virtual void fill_rect(int x, int y, int width, int height) = 0;
  void fill_rect(Rectangle const &rect) {
    fill_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual Rectangle get_clip_rect() const = 0;
  virtual void set_clip_rect(Rectangle const &rect) = 0;
  void set_clip_rect(int x, int y, int width, int height) {
    set_clip_rect( { x, y, width, height });
  }

  virtual bool hit_clip_rect(int x, int y, int width, int height) const = 0;
  bool hit_clip_rect(Rectangle const &rect) const {
    return hit_clip_rect(rect.x, rect.y, rect.width, rect.height);
  }

  virtual Color get_foreground_color() const = 0;
  virtual void set_foreground_color(Color const &color) = 0;
  void set_foreground_color(std::optional<Color> const &color) {
    if (color) {
      set_foreground_color(color.value());
    }
  }

  virtual Color get_background_color() const = 0;
  virtual void set_background_color(Color const &color) = 0;
  void set_background_color(std::optional<Color> const &color) {
    if (color) {
      set_background_color(color.value());
    }
  }

  virtual Font get_font() const = 0;
  virtual void set_font(Font const &font) = 0;

  virtual Stroke get_stroke() const = 0;
  virtual void set_stroke(Stroke stroke) = 0;

  virtual void translate(int dx, int dy) = 0;
};

}
