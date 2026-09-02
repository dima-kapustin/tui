#include <tui++/terminal/graphic/GraphicScreen.h>
#include <tui++/terminal/graphic/GraphicGraphics.h>
#include <tui++/terminal/Terminal.h>

#include <tui++/Window.h>

#include <tui++/terminal/graphic/GraphicEncoder.h>

#include <tui++/lookandfeel/graphic/GraphicLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <chrono>
#include <memory>
#include <algorithm>

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

GraphicScreen::GraphicScreen() {
  this->look_and_feel = std::make_shared<laf::GraphicLookAndFeel>();

  // The terminal reports its cell size in pixels (CSI 16 t); use it so every
  // image is emitted at the size the terminal actually displays. The default
  // font fills one cell: its size is the cell width in pixels.
  if (auto cell_size = terminal.query_cell_size()) {
    this->cell_width = std::max(4, cell_size->width);
    this->cell_height = std::max(4, cell_size->height);
  }
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

void GraphicScreen::resize_buffer() {
  this->pixels.assign(get_pixel_width() * get_pixel_height() * 3, 0);
  // The caller repaints (and therefore re-dirties) whatever needs to be
  // emitted, so the cleared buffer must not itself mark the screen dirty:
  // an initial full-screen dirty rect would make the first flush encode the
  // unpainted black tail below the content.
  this->dirty = { };
  this->has_dirty = false;
}

void GraphicScreen::mark_dirty(Rectangle const &rect) {
  if (rect.empty()) {
    return;
  }
  this->dirty = this->has_dirty ? this->dirty | rect : rect;
  this->has_dirty = true;
}

void GraphicScreen::move_cursor_to(int line, int column) {
  terminal << "\x1b["sv << line << ';' << column << 'H';
}

void GraphicScreen::fill_pixels(Rectangle const &rect, Color const &color) {
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

void GraphicScreen::blit_glyph(int x, int y, uint8_t const *rows, int width, int height, std::optional<Color> const &foreground, std::optional<Color> const &background) {
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

void GraphicScreen::clear() {
  std::fill(this->pixels.begin(), this->pixels.end(), 0);
  mark_dirty({ 0, 0, get_pixel_width(), get_pixel_height() });
}

void GraphicScreen::refresh() {
  auto g = GraphicGraphics { *this };
  paint(g);
  flush();
}

void GraphicScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_pixel_width(), get_pixel_height() };
  if (region.empty()) {
    return;
  }

  // Paint the tree with a graphics clipped to the region: every draw is
  // clipped to it, so the dirty rect (and therefore the encoded sixel image)
  // stays limited to the damaged area.
  auto g = GraphicGraphics { *this, region, 0, 0 };
  paint(g);
  flush();
}

void GraphicScreen::run_event_loop() {
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

std::unique_ptr<Graphics> GraphicScreen::get_graphics() {
  return std::make_unique<GraphicGraphics>(*this);
}

std::unique_ptr<Graphics> GraphicScreen::get_graphics(Rectangle const &clip) {
  return std::make_unique<GraphicGraphics>(*this, Rectangle { 0, 0, clip.width, clip.height }, clip.x, clip.y);
}

void GraphicScreen::flush() {
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

  auto encode_t0 = std::chrono::steady_clock::now();
  auto data = GraphicEncoder::encode(this->pixels.data() + (rect.y * get_pixel_width() + rect.x) * 3, rect.width, rect.height, get_pixel_width());
  auto encode_t1 = std::chrono::steady_clock::now();
  this->last_encode_ms = std::chrono::duration<double, std::milli>(encode_t1 - encode_t0).count();

  // One combined write: move to the region origin, emit the image, then park
  // the cursor back at the top-left so the next image is placed from a known
  // position. A single write and a single flush per frame keeps the ConPTY
  // round-trips to a minimum.
  std::string out;
  out.reserve(data.size() + 32);
  out += "\x1b[";
  out += std::to_string(rect.y / this->cell_height + 1);
  out += ';';
  out += std::to_string(rect.x / this->cell_width + 1);
  out += 'H';
  out += data;
  out += "\x1b[1;1H";

  auto write_t0 = std::chrono::steady_clock::now();
  terminal << out;
  terminal.flush();
  auto write_t1 = std::chrono::steady_clock::now();
  this->last_write_ms = std::chrono::duration<double, std::milli>(write_t1 - write_t0).count();
  this->last_bytes = data.size();
  ++this->flush_count;

  this->has_dirty = false;
  this->dirty = { };
}

}
