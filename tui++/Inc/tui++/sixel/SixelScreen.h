#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <tui++/Color.h>
#include <tui++/Screen.h>

namespace tui {

class SixelGraphics;

// A screen that rasterizes the component tree into a pixel framebuffer and
// outputs it using the DEC sixel graphics protocol. Its size (and therefore
// the component layout) is measured in pixels, one terminal cell being
// CELL_WIDTH x CELL_HEIGHT pixels.
class SixelScreen: public Screen {
  using base = Screen;

public:
  // Rasterization size of one terminal cell, in pixels.
  constexpr static int CELL_WIDTH = 8;
  constexpr static int CELL_HEIGHT = 16;

private:
  std::vector<uint8_t> pixels; // RGB, 3 bytes per pixel, row-major

  Rectangle dirty;
  bool has_dirty = false;

private:
  void resize_buffer();
  void mark_dirty(Rectangle const &rect);

  void move_cursor_to(int line, int column);

public:
  SixelScreen();

  virtual void run_event_loop() override;

  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const &clip) override;

  virtual void refresh() override;

  int get_pixel_width() const {
    return this->size.width;
  }

  int get_pixel_height() const {
    return this->size.height;
  }

  void fill_pixels(Rectangle const &rect, Color const &color);

  // Blits an 8x8 monochrome glyph at pixel (x, y). `glyph` points to 8 rows,
  // bit 7 of each byte being the leftmost pixel. Set bits are painted with the
  // foreground color, unset bits with the background color when one is present
  // (otherwise they are left untouched).
  void blit_glyph(int x, int y, uint8_t const *glyph, std::optional<Color> const &foreground, std::optional<Color> const &background);

  void clear();
  void flush();
};

}
