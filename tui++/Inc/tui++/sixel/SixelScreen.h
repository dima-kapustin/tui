#pragma once

#include <memory>
#include <vector>

#include <tui++/Color.h>
#include <tui++/Screen.h>

namespace tui {

class SixelGraphics;

// A screen that rasterizes the component tree into a pixel framebuffer
// (CELL_WIDTH x CELL_HEIGHT pixels per terminal cell) and outputs it using
// the DEC sixel graphics protocol.
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
    return this->size.width * CELL_WIDTH;
  }

  int get_pixel_height() const {
    return this->size.height * CELL_HEIGHT;
  }

  void fill_pixels(Rectangle const &rect, Color const &color);

  void clear();
  void flush();
};

}
