#include <tui++/terminal/sixel/SixelScreen.h>
#include <tui++/terminal/sixel/SixelGraphics.h>
#include <tui++/terminal/Terminal.h>

#include <tui++/Window.h>

#include <tui++/terminal/sixel/SixelEncoder.h>

#include <tui++/lookandfeel/sixel/SixelLookAndFeel.h>
#include <tui++/TextMetrics.h>

#include <tui++/Font.h>

#include <tui++/util/log.h>

#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <algorithm>
#include <utility>

using namespace std::string_view_literals;

namespace tui {

constexpr std::chrono::milliseconds WAIT_EVENT_TIMEOUT { 30 };

// The most events dispatched per event-loop iteration (see TextScreen's loop
// for why the batch is capped): a burst is dispatched together and the loop
// then repaints once, covering the whole batch.
constexpr int MAX_EVENTS_PER_TICK = 64;

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
  // The last text row of the terminal is unusable for sixel: an image whose
  // bottom band lands on it advances the text cursor one row past the screen
  // and Windows Terminal scrolls the buffer (every resize then jumps and
  // every following image is drawn one row off). Keep the screen one row
  // short of the bottom; the frame laid out over it gets its border on all
  // four sides and nothing ever triggers the scroll.
  this->size = { size.width * this->cell_width, std::max(1, size.height - 1) * this->cell_height };
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
  this->sent.assign(get_pixel_width() * get_pixel_height() * 3, 0);
  this->sent_valid.assign((std::size_t(get_pixel_width()) * get_pixel_height() + 7) / 8, 0);
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
  // Pixels drawn while a repaint pass is open are covered by the pass's
  // region rects (the graphics are clipped to them), so this union is only
  // encoded by flush() -- the direct, non-pass path.
  this->dirty = this->has_dirty ? this->dirty | rect : rect;
  this->has_dirty = true;
}

void SixelScreen::move_cursor_to(int line, int column) {
  terminal << "\x1b["sv << line << ';' << column << 'H';
}

// Returns the byte mask of the sent_valid bits covering pixels [first, last)
// of one pixel row, or 0 when the span is empty. Bits beyond the row are not
// included, so a full span can be checked with a single masked load.
static uint8_t row_bit_mask(int first, int last) {
  if (first >= last) {
    return 0;
  }
  auto b0 = size_t(first) >> 3;
  auto b1 = (size_t(last) - 1) >> 3;
  if (b0 == b1) {
    auto lo = uint8_t(0xFFu << (size_t(first) & 7));
    auto hi = uint8_t(0xFFu >> (7 - ((size_t(last) - 1) & 7)));
    return uint8_t(lo & hi);
  }
  return 0xFF;
}

bool SixelScreen::sent_matches(Rectangle const &rect) const {
  if (rect.empty()) {
    return false;
  }
  auto pw = get_pixel_width();
  auto const *px = this->pixels.data() + (std::size_t(rect.y) * pw + rect.x) * 3;
  auto const *sn = this->sent.data() + (std::size_t(rect.y) * pw + rect.x) * 3;
  auto const *vl = this->sent_valid.data();
  for (auto y = rect.y; y < rect.bottom(); ++y, px += pw * 3, sn += pw * 3) {
    // The row's span may cut through its first and last validity bytes.
    auto bit0 = std::size_t(y) * pw + rect.x;
    auto bit1 = std::size_t(y) * pw + rect.right();
    auto b0 = bit0 >> 3;
    auto b1 = (bit1 - 1) >> 3;
    auto mask = row_bit_mask(rect.x, rect.right());
    if (b0 == b1) {
      if ((vl[b0] & mask) != mask) {
        return false;
      }
    } else {
      if ((vl[b0] & uint8_t(0xFFu << (rect.x & 7))) != uint8_t(0xFFu << (rect.x & 7)) or (vl[b1] & uint8_t(0xFFu >> (7 - ((rect.right() - 1) & 7)))) != uint8_t(0xFFu >> (7 - ((rect.right() - 1) & 7)))) {
        return false;
      }
      for (auto b = b0 + 1; b < b1; ++b) {
        if (vl[b] != 0xFF) {
          return false;
        }
      }
    }
    if (std::memcmp(px, sn, std::size_t(rect.width) * 3) != 0) {
      return false;
    }
  }
  return true;
}

void SixelScreen::store_sent(Rectangle const &rect) {
  auto pw = get_pixel_width();
  auto *px = this->pixels.data() + (std::size_t(rect.y) * pw + rect.x) * 3;
  auto *sn = this->sent.data() + (std::size_t(rect.y) * pw + rect.x) * 3;
  auto *vl = this->sent_valid.data();
  for (auto y = rect.y; y < rect.bottom(); ++y, px += pw * 3, sn += pw * 3) {
    std::memcpy(sn, px, std::size_t(rect.width) * 3);
    auto bit0 = std::size_t(y) * pw + rect.x;
    auto bit1 = std::size_t(y) * pw + rect.right();
    auto b0 = bit0 >> 3;
    auto b1 = (bit1 - 1) >> 3;
    if (b0 == b1) {
      vl[b0] |= row_bit_mask(rect.x, rect.right());
    } else {
      vl[b0] |= uint8_t(0xFFu << (rect.x & 7));
      for (auto b = b0 + 1; b < b1; ++b) {
        vl[b] = 0xFF;
      }
      vl[b1] |= uint8_t(0xFFu >> (7 - ((rect.right() - 1) & 7)));
    }
  }
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
  // A full repaint covers every pending damaged region, so drop them; any
  // queued repaint invocation then becomes a no-op instead of repainting the
  // whole screen a second time.
  this->damaged_regions.clear();

  auto g = SixelGraphics { *this };
  paint(g);
  flush();
}

void SixelScreen::on_window_removed(Rectangle const &area) {
  // The removed window's pixels were painted over the windows beneath it, so
  // after it is gone the repaint of those windows only overwrites the pixels
  // they draw. Reset the area to the cleared (black) buffer state and drop
  // its sent bits before that repaint runs -- the flush then re-encodes
  // whatever the repaint leaves empty, erasing the window from the terminal
  // instead of leaving its stale pixels on screen.
  auto rect = area & Rectangle { 0, 0, get_pixel_width(), get_pixel_height() };
  if (rect.empty()) {
    return;
  }
  auto pw = get_pixel_width();
  for (auto y = rect.y; y < rect.bottom(); ++y) {
    std::memset(this->pixels.data() + (std::size_t(y) * pw + rect.x) * 3, 0, std::size_t(rect.width) * 3);

    // The sent mirror still shows the removed window, so clear the valid
    // bits of the area: a flush must re-encode it even when the repaint
    // below paints nothing over it.
    auto bit0 = std::size_t(y) * pw + rect.x;
    auto bit1 = std::size_t(y) * pw + rect.right();
    auto b0 = bit0 >> 3;
    auto b1 = (bit1 - 1) >> 3;
    if (b0 == b1) {
      this->sent_valid[b0] &= uint8_t(~row_bit_mask(rect.x, rect.right()));
    } else {
      this->sent_valid[b0] &= uint8_t(~(0xFFu << (rect.x & 7)));
      for (auto b = b0 + 1; b < b1; ++b) {
        this->sent_valid[b] = 0;
      }
      this->sent_valid[b1] &= uint8_t(~(0xFFu >> (7 - ((rect.right() - 1) & 7))));
    }
  }
  mark_dirty(rect);
}

void SixelScreen::repaint_region(Rectangle const &rect) {
  auto region = rect & Rectangle { 0, 0, get_pixel_width(), get_pixel_height() };
  if (region.empty()) {
    return;
  }

  // Paint the tree with a graphics clipped to the region: every draw is
  // clipped to it, so the pixels it changes lie inside the region rect.
  auto g = SixelGraphics { *this, region, 0, 0 };
  paint(g);

  if (this->in_repaint_pass) {
    // The repaint pass encodes every damaged rect together at its end, one
    // image per rect, with a single write and flush.
    add_pass_rect(region);
  } else {
    // No pass open (e.g. the screen's own immediate repaints): flush now.
    flush();
  }
}

void SixelScreen::repaint_pass_begin() {
  this->in_repaint_pass = true;
  this->pass_rects.clear();
}

void SixelScreen::repaint_pass_end() {
  this->in_repaint_pass = false;
  auto rects = std::exchange(this->pass_rects, { });
  // The pixels painted during the pass lie inside the recorded region rects
  // (each paint was clipped to its region), so the union accumulated by
  // mark_dirty is fully covered by them.
  this->dirty = { };
  this->has_dirty = false;
  write_images(rects);
}

void SixelScreen::add_pass_rect(Rectangle const &rect) {
  if (rect.empty()) {
    return;
  }

  auto area = [](Rectangle const &r) {
    return 1LL * r.width * r.height;
  };

  // Merge into the region whose union with the new rect grows least, but
  // only when the union stays cheap (same heuristic as Screen::add_damage);
  // otherwise keep the rects separate so two distant small repaints do not
  // become one tall image that re-encodes everything between them.
  constexpr size_t MAX_RECTS = 8;
  constexpr long long MERGE_ALLOWANCE = 2;

  auto best = this->pass_rects.size();
  auto best_area = std::numeric_limits<long long>::max();
  auto rect_area = area(rect);
  for (auto i = size_t { 0 }; i < this->pass_rects.size(); ++i) {
    auto merged = area(this->pass_rects[i] | rect);
    if (merged < best_area) {
      best_area = merged;
      best = i;
    }
  }

  auto can_merge = false;
  if (best < this->pass_rects.size()) {
    can_merge = best_area <= (rect_area + area(this->pass_rects[best])) * MERGE_ALLOWANCE;
  }

  if (can_merge) {
    this->pass_rects[best] |= rect;
  } else if (this->pass_rects.size() < MAX_RECTS) {
    this->pass_rects.emplace_back(rect);
  } else {
    // A pathological burst: bound the list by merging into the cheapest.
    this->pass_rects[best] |= rect;
  }
}

void SixelScreen::run_event_loop() {
  event_dispatching_thread_id = std::this_thread::get_id();

  // Coalesce repaint requests onto a frame clock (see TextScreen::run_event_loop).
  this->repaint_interval = std::chrono::milliseconds { 16 };

  auto size = this->size;
  while (not this->quit) {
    terminal.read_events();

    // The terminal screen owns the physical display and clears it when the
    // terminal is resized; detect the resize here and repaint the windows.
    // The last terminal row is excluded (see the constructor comment).
    auto ts = terminal.get_size();
    auto pixel_size = Dimension { ts.width * this->cell_width, std::max(1, ts.height - 1) * this->cell_height };
    if (pixel_size != size) {
      log_resize_ln("sixel screen " << size.width << 'x' << size.height << " px -> " << pixel_size.width << 'x' << pixel_size.height << " px");
      size = pixel_size;
      this->size = pixel_size;
      resize_buffer();

      // Top-level windows track the screen size so the layout fills the new
      // terminal instead of leaving stale, mis-sized frames behind. Popup
      // windows keep their own size and position (see TextScreen::resized).
      {
        std::unique_lock lock(this->windows_mutex);
        log_resize_ln("screen resize: " << this->windows.size() << " top-level window(s) to " << pixel_size.width << 'x' << pixel_size.height);
        for (auto &&window : this->windows) {
          if (window->get_type() != WindowType::POPUP) {
            window->set_size(pixel_size);
          }
        }
      }

      // The terminal still displays the previous sixel image; erase it so a
      // smaller new image does not leave stale pixels around it.
      terminal << "\x1b[2J\x1b[1;1H"sv;
      terminal.flush();

      refresh();
    }

    // Dispatch a bounded batch of events. Repainting is not a side effect of
    // the loop: repaint() requests accumulate damaged regions and schedule a
    // single repaint (on the queue, or on the frame clock -- see
    // Screen::add_damage). The batch cap keeps a burst (or a self-reposting
    // timer) from starving the loop. The wait is shortened to the pending
    // repaint's due time, so an idle screen still paints at its frame
    // boundary.
    auto wait = WAIT_EVENT_TIMEOUT;
    auto now = std::chrono::steady_clock::now();
    if (this->repaint_event_pending and this->repaint_due <= now + wait) {
      wait = std::chrono::duration_cast<std::chrono::milliseconds>(this->repaint_due - now);
      wait = std::max(wait, std::chrono::milliseconds::zero());
    }
    auto event = this->event_queue.pop(wait);
    for (auto n = 0; event and n < MAX_EVENTS_PER_TICK; ++n) {
      dispatch_event(*event);
      event = this->event_queue.pop(std::chrono::milliseconds::zero());
    }

    // Paint the frame once its boundary is due.
    repaint_if_due();
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
  auto rects = std::vector<Rectangle> { this->dirty };
  this->dirty = { };
  this->has_dirty = false;
  write_images(rects);
}

void SixelScreen::write_images(std::vector<Rectangle> const &rects) {
  auto encode_t0 = std::chrono::steady_clock::now();
  std::string out;
  auto total_bytes = size_t { 0 };

  for (auto const &dirty : rects) {
    // The sixel image is addressed in whole terminal cells, so align the
    // damaged rect to cell boundaries before encoding it.
    auto rect = dirty;
    auto left = rect.x / this->cell_width * this->cell_width;
    auto top = rect.y / this->cell_height * this->cell_height;
    auto right = std::min((rect.right() + this->cell_width - 1) / this->cell_width * this->cell_width, get_pixel_width());
    auto bottom = std::min((rect.bottom() + this->cell_height - 1) / this->cell_height * this->cell_height, get_pixel_height());

    // After an image the terminal moves its text cursor to the lower-left
    // corner of the graphic: one row below the image's bottom band, at the
    // image's left column. There is no horizontal advance, so the right edge
    // needs no protection. The screen itself already excludes the terminal's
    // last row (see the constructor), so the bottom band of any image sits
    // above it and the vertical advance never leaves the screen: no buffer
    // scroll, and the frame border renders on all four sides.

    if (bottom <= top or right <= left) {
      continue;
    }
    rect = { left, top, right - left, bottom - top };

    // The terminal silently truncates any image wider or taller than its
    // limit (xterm's maxGraphicSize, default 1000x1000); the first full draw
    // lost everything right of the limit to exactly this. Slice the damaged
    // rect into tiles no larger than the limit, each on the cell lattice so
    // every cursor position stays exact; adjacent tiles abut, so the seams
    // never show. Terminals without a limit (Windows Terminal) keep a single
    // image.
    auto tile_w = rect.width;
    auto tile_h = rect.height;
    if (this->max_graphic_size) {
      tile_w = std::max(this->cell_width, this->max_graphic_size->width / this->cell_width * this->cell_width);
      tile_h = std::max(this->cell_height, this->max_graphic_size->height / this->cell_height * this->cell_height);
    }

    for (auto ty = rect.y; ty < rect.bottom(); ty += tile_h) {
      auto th = std::min(tile_h, rect.bottom() - ty);
      for (auto tx = rect.x; tx < rect.right(); tx += tile_w) {
        auto tw = std::min(tile_w, rect.right() - tx);
        auto tile = Rectangle { tx, ty, tw, th };

        // No-op repaint: the terminal already displays these exact pixels, so
        // encoding and writing them again is pure cost (a no-op tick used to
        // stream the region every time). The mirror is only trusted where
        // pixels were actually sent.
        if (sent_matches(tile)) {
          ++this->skipped_tiles;
          continue;
        }
        ++this->emitted_tiles;

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
        store_sent(tile);
      }
    }
  }
  auto encode_t1 = std::chrono::steady_clock::now();
  this->last_encode_ms = std::chrono::duration<double, std::milli>(encode_t1 - encode_t0).count();

  log_repaint_ln(this->emitted_tiles << " tile(s) emitted, " << this->skipped_tiles << " skipped (" << total_bytes << " B)");
  this->skipped_tiles = 0;
  this->emitted_tiles = 0;

  if (out.empty()) {
    return;
  }

  out += "\x1b[1;1H";
  auto write_t0 = std::chrono::steady_clock::now();
  terminal << out;
  terminal.flush();
  auto write_t1 = std::chrono::steady_clock::now();
  this->last_write_ms = std::chrono::duration<double, std::milli>(write_t1 - write_t0).count();
  this->last_bytes = total_bytes;
  ++this->flush_count;
}

}
