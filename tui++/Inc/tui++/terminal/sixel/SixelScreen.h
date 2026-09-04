#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include <tui++/Color.h>
#include <tui++/Screen.h>
#include <tui++/TextMetrics.h>

namespace tui {

class SixelGraphics;

// A screen that rasterizes the component tree into a pixel framebuffer and
// outputs it using the DEC sixel graphics protocol. Its size (and therefore
// the component layout) is measured in pixels, one terminal cell being
// CELL_WIDTH x CELL_HEIGHT pixels.
class SixelScreen: public Screen {
  using base = Screen;

public:
  // Default rasterization size of one terminal cell, in pixels. The actual
  // cell size is queried from the terminal at construction (see
  // get_cell_width / get_cell_height) and these are only the fallback.
  constexpr static int CELL_WIDTH = SIXEL_CELL_WIDTH;
  constexpr static int CELL_HEIGHT = SIXEL_CELL_HEIGHT;

private:
  std::vector<uint8_t> pixels; // RGB, 3 bytes per pixel, row-major

  // Mirror of the pixels the terminal currently displays, with one valid bit
  // per pixel: a dirty rect whose pixels still equal the mirror was already
  // sent and can be skipped without re-encoding (no-op repaints -- same state
  // repainted by an event -- currently stream the whole region for nothing).
  std::vector<uint8_t> sent;
  std::vector<uint8_t> sent_valid; // 1 bit per pixel: 1 = the terminal shows it

  // Union of the pixels drawn outside a repaint pass (direct graphics
  // flushes, refresh); encoded by flush().
  Rectangle dirty;
  bool has_dirty = false;

  // Repaint-pass state: while a pass is open (Screen::repaint_damaged),
  // repaint_region() only records its rect and the pass end encodes each
  // rect as its own image in one write+flush. The rects are kept separate
  // on purpose: the union of two distant small rects would span (and
  // re-encode) everything between them, at ~10x the cost of the two images.
  bool in_repaint_pass = false;
  std::vector<Rectangle> pass_rects;

  std::shared_ptr<laf::LookAndFeel> look_and_feel;
  std::shared_ptr<TextMetrics> text_metrics;

  // The terminal's real cell size in pixels (CSI 16 t); falls back to
  // CELL_WIDTH / CELL_HEIGHT when the terminal does not answer.
  int cell_width = SIXEL_CELL_WIDTH;
  int cell_height = SIXEL_CELL_HEIGHT;

  // The largest sixel image the terminal will display, in pixels (xterm's
  // maxGraphicSize, default 1000x1000; empty when the terminal has no such
  // limit, e.g. Windows Terminal). The terminal silently truncates any
  // larger image, so every flush slices its dirty region into tiles no
  // larger than this instead of shrinking the screen: the layout keeps the
  // full terminal area and nothing is ever cut off.
  std::optional<Dimension> max_graphic_size;

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

  // True when the terminal already displays exactly the pixels of `rect`
  // (every pixel marked sent, and the buffer equal to the mirror).
  bool sent_matches(Rectangle const &rect) const;

  // Records `rect` as displayed: copies the pixels into the sent mirror and
  // marks them valid.
  void store_sent(Rectangle const &rect);

  // Records one damaged rect of the open repaint pass, merging it into the
  // existing rects when the union stays cheap (see Screen::add_damage) and
  // bounding everything together once the list grows too long.
  void add_pass_rect(Rectangle const &rect);

  // Encodes each rect as one (possibly tiled) sixel image, placed by cursor
  // moves, and writes them all with a single flush. Clears the flush state.
  void write_images(std::vector<Rectangle> const &rects);

  void move_cursor_to(int line, int column);

public:
  SixelScreen();

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
  // a graphics clipped to the region. The pixels it touches are flushed at
  // the end of the repaint pass (one combined write), or immediately when no
  // pass is open.
  virtual void repaint_region(Rectangle const &rect) override;

  virtual void repaint_pass_begin() override;
  virtual void repaint_pass_end() override;

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

  std::optional<Dimension> get_max_graphic_size() const {
    return this->max_graphic_size;
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
