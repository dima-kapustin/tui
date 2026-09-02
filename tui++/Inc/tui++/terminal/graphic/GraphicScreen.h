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
  // Default rasterization size of one terminal cell, in pixels. The actual
  // cell size is queried from the terminal at construction (see
  // get_cell_width / get_cell_height) and these are only the fallback.
  constexpr static int CELL_WIDTH = GRAPHIC_CELL_WIDTH;
  constexpr static int CELL_HEIGHT = GRAPHIC_CELL_HEIGHT;

private:
  std::vector<uint8_t> pixels; // RGB, 3 bytes per pixel, row-major

  Rectangle dirty;
  bool has_dirty = false;

  std::shared_ptr<laf::LookAndFeel> look_and_feel;
  std::shared_ptr<TextMetrics> text_metrics;

  // The terminal's real cell size in pixels (CSI 16 t); falls back to
  // CELL_WIDTH / CELL_HEIGHT when the terminal does not answer.
  int cell_width = GRAPHIC_CELL_WIDTH;
  int cell_height = GRAPHIC_CELL_HEIGHT;

  // Timing of the last flush: how long the sixel encoding took, how long the
  // write to the terminal took, and how many bytes were emitted. Used by the
  // interactive editor's benchmark mode to locate the slow phase.
  double last_encode_ms = 0;
  double last_write_ms = 0;
  size_t last_bytes = 0;

  // How many images have been flushed since the screen was created.
  size_t flush_count = 0;

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

  // Repaints only `rect` (screen coordinates): paints the component tree with
  // a graphics clipped to the region and flushes just the pixels it touches.
  virtual void repaint_region(Rectangle const &rect) override;

  // The mouse is reported by the terminal in text cells; this screen lays
  // components out in pixels. Use the centre of the reported cell: the click
  // can be anywhere inside it, and the top-left corner would bias every hit
  // toward the cell above and to the left.
  virtual Point convert_mouse_point(int x, int y) const override {
    return { x * this->cell_width + this->cell_width / 2, y * this->cell_height + this->cell_height / 2 };
  }

  int get_pixel_width() const {
    return this->size.width;
  }

  int get_pixel_height() const {
    return this->size.height;
  }

  // The terminal's real cell size in pixels (falls back to the defaults).
  int get_cell_width() const {
    return this->cell_width;
  }

  int get_cell_height() const {
    return this->cell_height;
  }

  double get_last_encode_ms() const {
    return this->last_encode_ms;
  }

  double get_last_write_ms() const {
    return this->last_write_ms;
  }

  size_t get_last_bytes() const {
    return this->last_bytes;
  }

  size_t get_flush_count() const {
    return this->flush_count;
  }

  void fill_pixels(Rectangle const &rect, Color const &color);

  // Blits a monochrome glyph at pixel (x, y). `rows` holds `height` rows of
  // (width + 7) / 8 bytes each; bit 7 of the first byte of a row is the
  // leftmost pixel. Set bits are painted with the foreground color, unset
  // bits with the background color when one is present (otherwise they are
  // left untouched).
  void blit_glyph(int x, int y, uint8_t const *rows, int width, int height, std::optional<Color> const &foreground, std::optional<Color> const &background);

  void clear();
  void flush();
};

}
