#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>

namespace tui {

class GraphicGraphics;

// A screen that rasterizes the component tree into a pixel framebuffer and
// outputs it using the DEC sixel graphics protocol. Its size (and therefore
// the component layout) is measured in pixels, one terminal cell being
// CELL_WIDTH x CELL_HEIGHT pixels.
class GraphicScreen: public Screen {
  using base = Screen;

public:
  // Rasterization size of one terminal cell, in pixels (single source of
  // truth: tui::GRAPHIC_CELL_WIDTH / GRAPHIC_CELL_HEIGHT).
  constexpr static int CELL_WIDTH = GRAPHIC_CELL_WIDTH;
  constexpr static int CELL_HEIGHT = GRAPHIC_CELL_HEIGHT;

private:
  std::vector<uint8_t> pixels; // RGB, 3 bytes per pixel, row-major

  Rectangle dirty;
  bool has_dirty = false;

  std::shared_ptr<laf::LookAndFeel> look_and_feel;
  std::shared_ptr<TextMetrics> text_metrics;

private:
  void resize_buffer();
  void mark_dirty(Rectangle const &rect);

  void move_cursor_to(int line, int column);

public:
  GraphicScreen();

  virtual void run_event_loop() override;

  virtual std::unique_ptr<Graphics> get_graphics() override;
  virtual std::unique_ptr<Graphics> get_graphics(Rectangle const &clip) override;

  virtual std::shared_ptr<laf::LookAndFeel> get_look_and_feel() const override {
    return this->look_and_feel;
  }

  virtual std::shared_ptr<TextMetrics> get_text_metrics() const override {
    return this->text_metrics;
  }

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
