#include <tui++/sixel/SixelScreen.h>
#include <tui++/sixel/SixelGraphics.h>
#include <tui++/terminal/Terminal.h>

#include <tui++/sixel/SixelEncoder.h>

#include <algorithm>
#include <chrono>

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

SixelScreen::SixelScreen() {
  this->size = terminal.get_size();
  resize_buffer();
}

void SixelScreen::resize_buffer() {
  this->pixels.assign(get_pixel_width() * get_pixel_height() * 3, 0);
  mark_dirty({ 0, 0, get_pixel_width(), get_pixel_height() });
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

void SixelScreen::clear() {
  std::fill(this->pixels.begin(), this->pixels.end(), 0);
  mark_dirty({ 0, 0, get_pixel_width(), get_pixel_height() });
}

void SixelScreen::refresh() {
  auto g = SixelGraphics { *this };
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
    if (terminal.get_size() != size) {
      size = terminal.get_size();
      this->size = size;
      resize_buffer();
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

  auto rect = this->dirty;
  auto data = SixelEncoder::encode(this->pixels.data() + (rect.y * get_pixel_width() + rect.x) * 3, rect.width, rect.height, get_pixel_width());

  move_cursor_to(rect.y / CELL_HEIGHT + 1, rect.x / CELL_WIDTH + 1);
  terminal << data;
  terminal.flush();

  this->has_dirty = false;
  this->dirty = { };
}

}
