#include <tui++/terminal/sixel/SixelScreen.h>
#include <tui++/terminal/sixel/SixelGraphics.h>
#include <tui++/terminal/Terminal.h>

#include <tui++/Window.h>

#include <tui++/terminal/sixel/SixelEncoder.h>

#include <tui++/lookandfeel/sixel/SixelLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <chrono>
#include <memory>
#include <algorithm>

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

SixelScreen::SixelScreen() {
  this->look_and_feel = std::make_shared<laf::SixelLookAndFeel>();

  // The terminal reports its cell size in pixels (CSI 16 t); use it so every
  // image is emitted at the size the terminal actually displays. The default
  // font fills one cell: its size is the cell width in pixels.
  if (auto cell_size = terminal.query_cell_size()) {
    this->cell_width = std::max(4, cell_size->width);
    this->cell_height = std::max(4, cell_size->height);
  }

  // The largest image the terminal will display (xterm's maxGraphicSize,
  // default 1000x1000): xterm silently truncates anything bigger, so every
  // flush tiles its dirty region to at most this size. Terminals without
  // the limit report nothing; xterm answers, or is identified via Device
  // Attributes and gets the default.
  this->max_graphic_size = terminal.query_graphics_geometry();

  laf::LookAndFeel::put<Font>("defaultFont", Font { "Monospaced", this->cell_width, Font::PLAIN });
  this->text_metrics = std::make_shared<PixelTextMetrics>(laf::LookAndFeel::get<Font>("defaultFont", Font { }));

  auto size = terminal.get_size();
  this->size = { size.width * this->cell_width, size.height * this->cell_height };
  resize_buffer();

  // Text printed before this screen took over (the unit tests) may have
  // scrolled the alternate buffer, so cursor row 1 is no longer the visible
  // top row. Re-enter the alternate buffer: it resets the scroll state, and
  // the first sixel image is then drawn at the top of the visible area --
  // otherwise the image (and every region repaint placed by cursor row) sits
  // above the visible area while the mouse reports viewport rows, and clicks
  // land the wrong distance below the drawn content.
  terminal << "\x1b[?1049l\x1b[?1049h"sv;
  terminal.flush();
}

void SixelScreen::resize_buffer() {
  this->pixels.assign(get_pixel_width() * get_pixel_height() * 3, 0);
  // The caller repaints (and therefore re-dirties) whatever needs to be
  // emitted, so the cleared buffer must not itself mark the screen dirty:
  // an initial full-screen dirty rect would make the first flush encode the
  // unpainted black tail below the content.
  this->dirty = { };
  this->has_dirty = false;
}

void SixelScreen::mark_dirty(Rectangle const &rect) {
  if (rect.empty()) {
    return;
  }
  this->dirty = this->has_dirty ? this->dirty | rect : rect;
  this->has_dirty = true;
}

void SixelScreen::move_cursor_to(int line, int column) {
  terminal << "\x1b["sv << line << ';' << column << 'H';
}

void SixelScreen::fill_pixels(Rectangle const &rect, Color const &color) {
  auto left = std::max(rect.x, 0);
  auto top = std::max(rect.y, 0);
  auto right = std::min(rect.right(), get_pixel_width());
  auto bottom = std::min(rect.bottom(), get_pixel_height());
  if (left >= right or top >= bottom) {
    return;
  }

  for (auto y = top; y < bottom; ++y) {
    auto *row = this->pixels.data() + (y * get_pixel_width() + left) * 3;
    for (auto x = left; x < right; ++x) {
      *row++ = color.red();
      *row++ = color.green();
      *row++ = color.blue();
    }
  }

  mark_dirty({ left, top, right - left, bottom - top });
}

void SixelScreen::blit_glyph(int x, int y, uint8_t const *rows, int width, int height, std::optional<Color> const &foreground, std::optional<Color> const &background) {
  auto left = std::max(x, 0);
  auto top = std::max(y, 0);
  auto right = std::min(x + width, get_pixel_width());
  auto bottom = std::min(y + height, get_pixel_height());
  if (left >= right or top >= bottom) {
    return;
  }

  auto row_bytes = (width + 7) / 8;
  for (auto py = top; py < bottom; ++py) {
    auto const *row = rows + (py - y) * row_bytes;
    for (auto px = left; px < right; ++px) {
      auto bit = px - x;
      auto color = (row[bit / 8] & (0x80 >> (bit % 8))) ? foreground : background;
      if (not color) {
        continue;
      }
      auto *p = this->pixels.data() + (py * get_pixel_width() + px) * 3;
      *p++ = color->red();
      *p++ = color->green();
      *p++ = color->blue();
    }
  }

  mark_dirty({ left, top, right - left, bottom - top });
}

void SixelScreen::clear() {
  std::fill(this->pixels.begin(), this->pixels.end(), 0);
  mark_dirty({ 0, 0, get_pixel_width(), get_pixel_height() });
}

void SixelScreen::refresh() {
  auto g = SixelGraphics { *this };
  paint(g);
  flush();
}

void SixelScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_pixel_width(), get_pixel_height() };
  if (region.empty()) {
    return;
  }

  // Paint the tree with a graphics clipped to the region: every draw is
  // clipped to it, so the dirty rect (and therefore the encoded sixel image)
  // stays limited to the damaged area.
  auto g = SixelGraphics { *this, region, 0, 0 };
  paint(g);
  flush();
}

void SixelScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  auto size = this->size;
  while (not this->quit) {
    terminal.read_events();

    // The terminal screen owns the physical display and clears it when the
    // terminal is resized; detect the resize here and repaint the windows.
    auto ts = terminal.get_size();
    auto pixel_size = Dimension { ts.width * this->cell_width, ts.height * this->cell_height };
    if (pixel_size != size) {
      size = pixel_size;
      this->size = pixel_size;
      resize_buffer();

      // Top-level windows track the screen size so the layout fills the new
      // terminal instead of leaving stale, mis-sized frames behind.
      {
        std::unique_lock lock(this->windows_mutex);
        for (auto &&window : this->windows) {
          window->set_size(pixel_size);
        }
      }

      // The terminal still displays the previous sixel image; erase it so a
      // smaller new image does not leave stale pixels around it.
      terminal << "\x1b[2J\x1b[1;1H"sv;
      terminal.flush();

      refresh();
    }

    if (auto event = this->event_queue.pop(WAIT_EVENT_TIMEOUT)) {
      dispatch_event(*event);
    }
  }
}

std::unique_ptr<Graphics> SixelScreen::get_graphics() {
  return std::make_unique<SixelGraphics>(*this);
}

std::unique_ptr<Graphics> SixelScreen::get_graphics(Rectangle const &clip) {
  return std::make_unique<SixelGraphics>(*this, Rectangle { 0, 0, clip.width, clip.height }, clip.x, clip.y);
}

void SixelScreen::flush() {
  if (not this->has_dirty) {
    return;
  }

  // The sixel image is addressed in whole terminal cells, so align the dirty
  // region to cell boundaries before encoding it.
  auto rect = this->dirty;
  auto left = rect.x / this->cell_width * this->cell_width;
  auto top = rect.y / this->cell_height * this->cell_height;
  auto right = std::min((rect.right() + this->cell_width - 1) / this->cell_width * this->cell_width, get_pixel_width());
  auto bottom = std::min((rect.bottom() + this->cell_height - 1) / this->cell_height * this->cell_height, get_pixel_height());

  // After an image the terminal advances the text cursor past its bottom-right
  // corner; an image reaching the screen's last row or column would push the
  // cursor off the visible area and scroll the buffer (the first full draw
  // scrolled up a line). Keep every image one cell short of the bottom-right
  // corner; layouts reserve the same margin, so this never cuts content.
  auto cells = terminal.get_size();
  if (cells.width > 1) {
    right = std::min(right, (cells.width - 1) * this->cell_width);
  }
  if (cells.height > 1) {
    bottom = std::min(bottom, (cells.height - 1) * this->cell_height);
  }

  if (bottom <= top or right <= left) {
    this->has_dirty = false;
    this->dirty = { };
    return;
  }
  rect = { left, top, right - left, bottom - top };

  // The terminal silently truncates any image wider or taller than its
  // limit (xterm's maxGraphicSize, default 1000x1000); the first full draw
  // lost everything right of the limit to exactly this. Slice the dirty
  // region into tiles no larger than the limit, each on the cell lattice
  // so every cursor position stays exact; adjacent tiles abut, so the seams
  // never show. Terminals without a limit (Windows Terminal) keep a single
  // image.
  auto tile_w = right - left;
  auto tile_h = bottom - top;
  if (this->max_graphic_size) {
    tile_w = std::max(this->cell_width, this->max_graphic_size->width / this->cell_width * this->cell_width);
    tile_h = std::max(this->cell_height, this->max_graphic_size->height / this->cell_height * this->cell_height);
  }

  auto encode_t0 = std::chrono::steady_clock::now();
  std::string out;
  out.reserve(rect.width * rect.height / 8 + 64);
  auto total_bytes = size_t { 0 };
  for (auto ty = top; ty < bottom; ty += tile_h) {
    auto th = std::min(tile_h, bottom - ty);
    for (auto tx = left; tx < right; tx += tile_w) {
      auto tw = std::min(tile_w, right - tx);
      auto data = SixelEncoder::encode(this->pixels.data() + (ty * get_pixel_width() + tx) * 3, tw, th, get_pixel_width());
      total_bytes += data.size();

      // Move to the tile origin and emit its image. One combined write and
      // a single flush per frame keeps the ConPTY round-trips to a minimum;
      // the cursor is parked back at the top-left after the last tile so
      // the next flush is placed from a known position.
      out += "\x1b[";
      out += std::to_string(ty / this->cell_height + 1);
      out += ';';
      out += std::to_string(tx / this->cell_width + 1);
      out += 'H';
      out += data;
    }
  }
  out += "\x1b[1;1H";
  auto encode_t1 = std::chrono::steady_clock::now();
  this->last_encode_ms = std::chrono::duration<double, std::milli>(encode_t1 - encode_t0).count();

  auto write_t0 = std::chrono::steady_clock::now();
  terminal << out;
  terminal.flush();
  auto write_t1 = std::chrono::steady_clock::now();
  this->last_write_ms = std::chrono::duration<double, std::milli>(write_t1 - write_t0).count();
  this->last_bytes = total_bytes;
  ++this->flush_count;

  this->has_dirty = false;
  this->dirty = { };
}

}
