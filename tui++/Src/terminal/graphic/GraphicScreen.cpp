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
  this->text_metrics = std::make_shared<PixelTextMetrics>(Font { });

  auto size = terminal.get_size();
  this->size = { size.width * CELL_WIDTH, size.height * CELL_HEIGHT };
  resize_buffer();
}

void GraphicScreen::resize_buffer() {
  this->pixels.assign(get_pixel_width() * get_pixel_height() * 3, 0);
  mark_dirty({ 0, 0, get_pixel_width(), get_pixel_height() });
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

void GraphicScreen::blit_glyph(int x, int y, uint8_t const *glyph, std::optional<Color> const &foreground, std::optional<Color> const &background) {
  auto left = std::max(x, 0);
  auto top = std::max(y, 0);
  auto right = std::min(x + 8, get_pixel_width());
  auto bottom = std::min(y + 8, get_pixel_height());
  if (left >= right or top >= bottom) {
    return;
  }

  for (auto py = top; py < bottom; ++py) {
    auto row = glyph[py - y];
    for (auto px = left; px < right; ++px) {
      auto color = (row & (0x80 >> (px - x))) ? foreground : background;
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

void GraphicScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  auto size = this->size;
  while (not this->quit) {
    terminal.read_events();

    // The terminal screen owns the physical display and clears it when the
    // terminal is resized; detect the resize here and repaint the windows.
    auto ts = terminal.get_size();
    auto pixel_size = Dimension { ts.width * CELL_WIDTH, ts.height * CELL_HEIGHT };
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
  auto left = rect.x / CELL_WIDTH * CELL_WIDTH;
  auto top = rect.y / CELL_HEIGHT * CELL_HEIGHT;
  auto right = std::min((rect.right() + CELL_WIDTH - 1) / CELL_WIDTH * CELL_WIDTH, get_pixel_width());
  auto bottom = std::min((rect.bottom() + CELL_HEIGHT - 1) / CELL_HEIGHT * CELL_HEIGHT, get_pixel_height());
  rect = { left, top, right - left, bottom - top };

  auto data = GraphicEncoder::encode(this->pixels.data() + (rect.y * get_pixel_width() + rect.x) * 3, rect.width, rect.height, get_pixel_width());

  move_cursor_to(rect.y / CELL_HEIGHT + 1, rect.x / CELL_WIDTH + 1);
  terminal << data;
  terminal.flush();

  // A sixel image advances the terminal's text cursor past its bottom edge;
  // park it back at the top-left so the next redraw does not scroll.
  move_cursor_to(1, 1);
  terminal.flush();

  this->has_dirty = false;
  this->dirty = { };
}

}
