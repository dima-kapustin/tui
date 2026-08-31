#pragma once

#include <tui++/Graphics.h>

namespace tui {

class SixelScreen;

// Rasterizes the drawing API into the SixelScreen's pixel framebuffer. All
// coordinates and sizes are in pixels: the SixelScreen reports its size in
// pixels, so components lay out and draw directly against individual pixels.
class SixelGraphics: public Graphics {
  SixelScreen &screen;

  int dx, dy;
  Rectangle clip;

  Stroke stroke = Stroke::LIGHT;

  Font font;

  std::optional<Color> foreground_color;
  std::optional<Color> background_color;

private:
  std::optional<Color> get_fill_color() const;
  std::optional<Color> get_line_color() const;

  // Returns the pixel rectangle (in this graphics' coordinates, offset by
  // dx/dy) clipped to the clip rect.
  Rectangle clipped(int x, int y, int width, int height) const;

  // Draws a solid pixel rectangle with the line color.
  void draw_pixel_rect(int x, int y, int width, int height);

  // Blits the glyph for `code` (from the embedded bitmap font) at pixel (x, y).
  void blit_glyph(char32_t code, int x, int y);

  int stroke_width() const;

public:
  SixelGraphics(SixelScreen &screen);
  SixelGraphics(SixelScreen &screen, const Rectangle &clip_rect, int dx, int dy);

public:
  virtual void clip_rect(int x, int y, int width, int height) override;

  virtual std::unique_ptr<Graphics> create();
  virtual std::unique_ptr<Graphics> create(int x, int y, int width, int height) override;

  virtual void draw_char(const Char &c, int x, int y, std::optional<Attributes> const &attributes = std::nullopt) override;

  virtual void draw_hline(int x, int y, int length, std::optional<Attributes> const &attributes = std::nullopt) override;

  virtual void draw_rect(int x, int y, int width, int height) override;

  virtual void draw_rounded_rect(int x, int y, int width, int height) override;

  virtual void draw_string(const std::string &str, int x, int y, std::optional<Attributes> const &attributes = std::nullopt) override;

  virtual void draw_vline(int x, int y, int length, std::optional<Attributes> const &attributes = std::nullopt) override;

  virtual void fill_rect(int x, int y, int width, int height) override;

  virtual Rectangle get_clip_rect() const override;
  virtual void set_clip_rect(const Rectangle &rect) override;

  virtual bool hit_clip_rect(int x, int y, int width, int height) const override;

  virtual std::optional<Color> const& get_foreground_color() const override;
  virtual void set_foreground_color(std::optional<Color> const &color) override;

  virtual std::optional<Color> const& get_background_color() const override;
  virtual void set_background_color(std::optional<Color> const &color) override;

  virtual Font get_font() const override;
  virtual void set_font(const Font &font) override;

  virtual Stroke get_stroke() const override;
  virtual void set_stroke(Stroke stroke) override;

  virtual void translate(int dx, int dy) override;

  void flush();
};

}
