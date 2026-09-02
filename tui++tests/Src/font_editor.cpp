// Interactive font editor for the graphic (sixel) terminal.
//
// Run with:  tui++tests fontedit
//
// Shows a 16x32 pixel grid of the current glyph. Click a cell to toggle its
// pixel (right-click clears), navigate glyphs with the arrows, and dump the
// corrected bitmap as the C hex rows that SixelGraphics::blit_glyph renders
// from (see Font16x32.h).
//
//   click            toggle a pixel (left button) / clear it (right button)
//   arrows           previous / next glyph
//   c                clear the current glyph
//   r                reset the current glyph to the built-in font
//   d                dump the current glyph as C rows of 32 x 2 hex bytes
//   D                dump the whole font table as C rows
//   q                quit and print every glyph that differs from the built-in
//
// Edits repaint only the affected regions (the toggled cell, the hex dump,
// the previews, ...) so a click re-encodes a few small sixel images instead
// of the whole screen. The grid cell is exactly one terminal cell (the unit
// the mouse is reported in) and the grid origin sits on the terminal's cell
// lattice, so every click lands on the pixel under the cursor. Terminals too
// short for the whole editor scroll the content with the mouse wheel.
// The File and Edit menus run the same commands as the keys below.
#include <tui++/Font.h>
#include <tui++/Char.h>
#include <tui++/Graphics.h>
#include <tui++/Component.h>
#include <tui++/Frame.h>
#include <tui++/Screen.h>

#include <tui++/Menu.h>
#include <tui++/MenuBar.h>
#include <tui++/MenuItem.h>

#include <tui++/border/EmptyBorder.h>

#include <tui++/terminal/Terminal.h>
#include <tui++/terminal/sixel/SixelScreen.h>
#include <tui++/terminal/sixel/Font16x32.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <numeric>
#include <thread>
#include <vector>

using namespace tui;

namespace tui {

class FontEditorPanel: public Component {
public:
  static constexpr int GLYPH_W = detail::FONT_WIDTH;  // 16
  static constexpr int GLYPH_H = detail::FONT_HEIGHT; // 32
  static constexpr int ROW_BYTES = GLYPH_W / 8;       // 2 bytes per row

  // Editor geometry (pixels). The grid cell pitch must be an exact multiple
  // of the terminal's cell (the unit the mouse reports in), or clicks can
  // never line up with the grid; see cell_size and hit_cell below.
  static constexpr int HEADER_FONT = 16;
  static constexpr int HEX_FONT = 12;
  static constexpr int GRID_X = 60;  // nominal; leaves room for the row labels
  static constexpr int GRID_Y = 140; // nominal; below the header + column labels

private:
  std::array<uint8_t, 128 * GLYPH_H * ROW_BYTES> font;     // working copy
  std::array<uint8_t, 128 * GLYPH_H * ROW_BYTES> original; // pristine built-in
  int glyph = 'A';
  int scroll_y = 0; // content scroll, always a whole number of cells

  uint8_t* current() {
    return &this->font[this->glyph * GLYPH_H * ROW_BYTES];
  }

  const uint8_t* current() const {
    return &this->font[this->glyph * GLYPH_H * ROW_BYTES];
  }

  bool get_pixel(const uint8_t *rows, int x, int y) const {
    return rows[y * ROW_BYTES + x / 8] & (0x80 >> (x % 8));
  }

  bool is_changed() const {
    auto *a = &this->font[this->glyph * GLYPH_H * ROW_BYTES];
    auto *b = &this->original[this->glyph * GLYPH_H * ROW_BYTES];
    return not std::equal(a, a + GLYPH_H * ROW_BYTES, b);
  }

  // fill_rect paints with the *background* color when one is set; the editor
  // draws the whole panel background first and paints with foreground colors
  // afterwards, so every fill clears the background first.
  static void fill(Graphics &g, int x, int y, int w, int h, Color color) {
    g.set_background_color(std::nullopt);
    g.set_foreground_color(color);
    g.fill_rect(x, y, w, h);
  }

  void blit_glyph_rows(Graphics &g, const uint8_t *rows, int x, int y, int scale, Color color) const {
    g.set_background_color(std::nullopt);
    g.set_foreground_color(color);
    // Fill horizontal runs of set pixels instead of one rect per pixel: the
    // overview strip alone holds tens of thousands of glyph pixels.
    for (auto ry = 0; ry < GLYPH_H; ++ry) {
      auto rx = 0;
      while (rx < GLYPH_W) {
        if (get_pixel(rows, rx, ry)) {
          auto start = rx;
          while (rx < GLYPH_W and get_pixel(rows, rx, ry)) {
            ++rx;
          }
          g.fill_rect(x + start * scale, y + ry * scale, (rx - start) * scale, scale);
        } else {
          ++rx;
        }
      }
    }
  }

public:
  FontEditorPanel() {
    std::copy_n(&detail::FONT16X32_BASIC[0][0], this->font.size(), this->font.begin());
    std::copy_n(&detail::FONT16X32_BASIC[0][0], this->original.size(), this->original.begin());
  }

  int get_glyph() const {
    return this->glyph;
  }

  // True when the current glyph differs from the built-in font.
  bool is_modified() const {
    return is_changed();
  }

  void prev_glyph() {
    this->glyph = (this->glyph == 0x20) ? 0x7E : this->glyph - 1;
  }

  void next_glyph() {
    this->glyph = (this->glyph == 0x7E) ? 0x20 : this->glyph + 1;
  }

  // Maps a pixel coordinate (panel space) to the clicked grid cell.
  //
  // The terminal reports the mouse in whole cells (10x20 virtual pixels on
  // Windows Terminal) and the screen maps that to the centre of the reported
  // cell, so the click position only ever lands on the terminal's cell
  // lattice. The grid pitch is exactly one cell and the grid origin sits on
  // the same lattice (see cell_size / grid_x / grid_y), so the division below
  // maps the click onto exactly the cell the user aimed at; a pitch that is
  // not a multiple of the cell would leave some rows unreachable and spill
  // clicks near cell boundaries onto the neighbours.
  bool hit_cell(int px, int py, int &gx, int &gy) const {
    auto s = step();
    auto x0 = grid_x();
    auto y0 = grid_y();
    gx = (px - x0) / s;
    gy = (py - y0) / s;
    return px >= x0 and px < x0 + GLYPH_W * s and py >= y0 and py < y0 + GLYPH_H * s;
  }

  void toggle_cell(int gx, int gy) {
    this->current()[gy * ROW_BYTES + gx / 8] ^= uint8_t(0x80 >> (gx % 8));
  }

  void clear_cell(int gx, int gy) {
    this->current()[gy * ROW_BYTES + gx / 8] &= uint8_t(~(0x80 >> (gx % 8)));
  }

  void clear_glyph() {
    std::fill_n(this->current(), GLYPH_H * ROW_BYTES, uint8_t(0));
  }

  void reset_glyph() {
    auto *src = &this->original[this->glyph * GLYPH_H * ROW_BYTES];
    std::copy_n(src, GLYPH_H * ROW_BYTES, this->current());
  }

  void dump_glyph() {
    dump_rows(this->glyph, this->current());
  }

  void dump_font() {
    for (auto code = 0; code < 128; ++code) {
      dump_rows(code, &this->font[code * GLYPH_H * ROW_BYTES]);
    }
  }

  void quit() {
    std::printf("FONT EDITOR RESULT -- glyphs changed from the built-in font:\n");
    for (auto code = 0; code < 128; ++code) {
      auto *a = &this->font[code * GLYPH_H * ROW_BYTES];
      auto *b = &this->original[code * GLYPH_H * ROW_BYTES];
      if (std::equal(a, a + GLYPH_H * ROW_BYTES, b)) {
        continue;
      }
      dump_rows(code, a);
    }
    std::fflush(stdout);
    std::exit(0);
  }

  // ---- damaged regions ----------------------------------------------------

  // Repaints the regions a grid edit changes: the toggled cell, the two
  // preview pixels, the row's hex bytes, and (only when the glyph's modified
  // state flipped) the status line and the glyph's strip cell. A click
  // therefore re-encodes a handful of tiny images, never the screen.
  void repaint_after_cell_edit(int gx, int gy, bool was_modified) {
    auto regions = std::vector<Rectangle> { grid_cell_rect(gx, gy), preview4_rect(gx, gy), preview1_rect(gx, gy), row_hex_rect(gy) };
    if (was_modified != is_modified()) {
      regions.push_back(status_rect());
      regions.push_back(strip_cell_rect(this->glyph));
    }
    repaint_regions(regions);
  }

  // Repaints the regions that a glyph-level edit changes (clear, reset or
  // navigating to another glyph). `previous_glyph` is the glyph shown before
  // the change (-1 when the glyph itself did not change).
  void repaint_after_glyph_edit(int previous_glyph) {
    auto regions = std::vector<Rectangle> { title_rect(), grid_rect(), status_rect(), preview_rect(), strip_cell_rect(this->glyph), row_hex_all_rect() };
    if (previous_glyph >= 0x20 and previous_glyph != this->glyph) {
      regions.push_back(strip_cell_rect(previous_glyph));
    }
    repaint_regions(regions);
  }

  // Scrolls the content by `delta` pixels (the caller passes whole cells so
  // the grid keeps its alignment with the mouse lattice). Returns true when
  // the scroll position changed.
  bool scroll_content(int delta) {
    auto next = std::clamp(this->scroll_y + delta, 0, scroll_max());
    if (next == this->scroll_y) {
      return false;
    }
    this->scroll_y = next;
    return true;
  }

  // Repaints the whole panel: every pixel of the content moves on a scroll.
  void repaint_all() {
    auto origin = this->get_location_on_screen();
    screen.repaint_region({ origin.x, origin.y, get_width(), get_height() });
  }

  // Repaints only the part of the panel that moves on a scroll: everything
  // below the fixed header band. The title lines and the column labels never
  // change when the content scrolls, so re-encoding them on every scroll step
  // would waste terminal-side decode time on pixels that do not move.
  void repaint_scrolled_area() {
    auto origin = this->get_location_on_screen();
    auto fixed = col_label_top() + label_line();
    auto top = origin.y + fixed;
    auto h = get_height() - fixed;
    if (h > 0) {
      screen.repaint_region({ origin.x, top, get_width(), h });
    }
  }

private:
  void repaint_regions(std::vector<Rectangle> const &regions) {
    // The regions are in the panel's coordinate space; repaint_region takes
    // screen coordinates, which are offset by every ancestor (the menu bar).
    auto origin = this->get_location_on_screen();
    for (auto const &r : regions) {
      screen.repaint_region({ r.x + origin.x, r.y + origin.y, r.width, r.height });
    }
  }

  // ---- geometry (derived from the panel size and the terminal cell) --------

  // One terminal cell in (virtual) pixels -- the unit both the sixel raster
  // and the mouse reports are addressed in.
  int cell_width() const {
    return static_cast<SixelScreen&>(screen).get_cell_width();
  }

  int cell_height() const {
    return static_cast<SixelScreen&>(screen).get_cell_height();
  }

  // Snaps a nominal position up to the next terminal-cell boundary in screen
  // space. Grid cell boundaries then coincide with the cell boundaries the
  // mouse reports, so the hit test in hit_cell is exact.
  int snap_to_cell_grid(int nominal, int unit, int screen_offset) const {
    return nominal + (unit - (nominal + screen_offset) % unit) % unit;
  }

  // The grid origin in panel space, snapped onto the terminal's cell lattice.
  int grid_x() const {
    return snap_to_cell_grid(GRID_X, cell_width(), get_location_on_screen().x);
  }

  // The unscrolled origin: the full content height is measured from here.
  int grid_y_origin() const {
    return snap_to_cell_grid(GRID_Y, cell_height(), get_location_on_screen().y);
  }

  // The visible origin: the content scrolls up by scroll_y, a whole number of
  // cells, so the alignment with the mouse lattice is preserved.
  int grid_y() const {
    return grid_y_origin() - this->scroll_y;
  }

  // ---- labels -------------------------------------------------------------

  int header_line() const {
    return 2 * HEADER_FONT;
  }

  // The labels use a font whose glyphs are exactly one grid cell tall, so a
  // row label and the per-row hex line up with their grid row.
  int label_font() const {
    return cell_height() / 2;
  }

  int label_line() const {
    return 2 * label_font(); // == cell_height()
  }

  int gutter() const {
    return 8;
  }

  // The fixed (non-scrolling) header block ends just above the column labels,
  // which sit just above the grid. grid_y_origin() is measured up from here.
  int header_top() const {
    return grid_y_origin() - 3 * header_line() - label_line() - 2 * gutter();
  }

  int col_label_top() const {
    return header_top() + 3 * header_line() + gutter();
  }

  int row_label_gutter() const {
    return 2 * label_font() + gutter(); // room for "31" + a gap
  }

  int per_row_hex_x() const {
    return grid_x() + grid_w() + gutter();
  }

  int per_row_hex_w() const {
    return 5 * label_font(); // "XX XX"
  }

  int per_row() const {
    return std::max(1, (get_width() - 16) / GLYPH_W);
  }

  int strip_rows() const {
    return (95 + per_row() - 1) / per_row();
  }

  // The grid cell is square and spans an exact number of terminal cells in
  // both directions, so every editor pixel lines up with the click lattice.
  // The smallest such cell is the lcm of the cell sides (20x20 on WT).
  int cell_size() const {
    return std::lcm(cell_width(), cell_height());
  }

  int step() const {
    return cell_size();
  }

  int grid_w() const {
    return GLYPH_W * step();
  }

  int grid_h() const {
    return GLYPH_H * step();
  }

  int dump_x() const {
    return per_row_hex_x() + per_row_hex_w() + 2 * gutter();
  }

  int strip_y() const {
    return grid_y() + grid_h() + 16;
  }

  // The full content height (unscrolled); the scroll bound derives from it.
  int content_h() const {
    return grid_y_origin() + grid_h() + 16 + strip_rows() * GLYPH_H + 8;
  }

  // How far the content can scroll: the overflow beyond the panel, keeping
  // one cell of slack at the bottom (the screen's flush never touches the
  // terminal's last row), rounded up to a whole number of cells so the grid
  // stays aligned with the mouse lattice while scrolling.
  int scroll_max() const {
    auto h = get_height();
    if (h <= 0) {
      return 0;
    }
    auto ch = cell_height();
    auto overflow = content_h() + ch - h;
    if (overflow <= 0) {
      return 0;
    }
    return (overflow + ch - 1) / ch * ch;
  }

  // The previews sit to the right of the grid; they are drawn only when
  // they fit within the panel width.
  bool side_area_fits() const {
    return dump_x() + 4 * GLYPH_W + 7 * HEX_FONT + GLYPH_W + 16 <= get_width();
  }

  Rectangle grid_cell_rect(int gx, int gy) const {
    return { grid_x() + gx * step(), grid_y() + gy * step(), cell_size(), cell_size() };
  }

  Rectangle grid_rect() const {
    return { grid_x(), grid_y(), grid_w(), grid_h() };
  }

  Rectangle title_rect() const {
    return { 16, header_top(), 40 * HEADER_FONT, 2 * HEADER_FONT };
  }

  Rectangle status_rect() const {
    return { 16, header_top() + 2 * header_line(), 30 * HEADER_FONT, 2 * HEADER_FONT };
  }

  // The 4x and 1x previews; the hex byte matrix that used to sit between
  // the grid and the previews is gone (every row's two bytes are already
  // shown next to the grid), so the previews moved up right below the grid.
  Rectangle preview_rect() const {
    return { this->dump_x() - 2, grid_y() + 2 * HEX_FONT - 2, 4 * GLYPH_W + 7 * HEX_FONT + GLYPH_W + 16, 4 * GLYPH_H + 6 };
  }

  // The changed pixel of the 4x preview (scale 4) and of the 1x preview.
  Rectangle preview4_rect(int gx, int gy) const {
    return { this->dump_x() + gx * 4, grid_y() + 2 * HEX_FONT + gy * 4, 4, 4 };
  }

  Rectangle preview1_rect(int gx, int gy) const {
    return { this->dump_x() + 4 * GLYPH_W + 12 + 3 * HEX_FONT + 8 + gx, grid_y() + 2 * HEX_FONT + GLYPH_H * 4 - 16 + gy, 1, 1 };
  }

  Rectangle strip_cell_rect(int code) const {
    auto i = code - 0x20;
    return { 8 + (i % per_row()) * GLYPH_W, this->strip_y() + (i / per_row()) * GLYPH_H, GLYPH_W, GLYPH_H };
  }

  // The column labels are fixed above the grid; the row labels and the
  // per-row hex scroll with it. These rects are used by the damaged-region
  // repaints (only the per-row hex actually changes on an edit, but the
  // whole gutters are repainted on a glyph-level edit).
  Rectangle column_label_rect() const {
    return { grid_x(), col_label_top(), grid_w(), label_line() };
  }

  Rectangle row_hex_rect(int gy) const {
    return { per_row_hex_x(), grid_y() + gy * step(), per_row_hex_w(), step() };
  }

  Rectangle row_hex_all_rect() const {
    return { per_row_hex_x(), grid_y(), per_row_hex_w(), GLYPH_H * step() };
  }

  // Prints one glyph as C source: a row of GLYPH_H * ROW_BYTES hex bytes.
  static void dump_rows(int code, const uint8_t *rows) {
    std::printf("// U+%04X '%c'\n", code, (code >= 0x20 and code < 0x7F) ? char(code) : '?');
    std::printf("{");
    for (auto i = 0; i < GLYPH_H * ROW_BYTES; ++i) {
      std::printf("0x%02X%s", rows[i], i < GLYPH_H * ROW_BYTES - 1 ? ", " : "");
    }
    std::printf("},\n");
    std::fflush(stdout);
  }

public:
  void paint(Graphics &g) override {
    auto w = get_width();
    auto h = get_height();
    if (w <= 0 or h <= 0) {
      return;
    }

    auto background = Color { 10, 10, 14 };
    auto text = Color { 215, 215, 215 };
    auto dim = Color { 120, 125, 135 };
    auto ink = Color { 225, 225, 230 };
    auto empty = Color { 34, 34, 42 };
    auto descender_zone = Color { 40, 40, 50 };
    auto baseline_color = Color { 70, 70, 85 };
    auto changed_color = Color { 225, 165, 45 };
    auto hex_color = Color { 150, 195, 255 };
    auto highlight = Color { 215, 180, 40 };
    auto on_highlight = Color { 10, 10, 14 };

    auto hline = 2 * HEADER_FONT;
    auto hexline = 2 * HEX_FONT;
    auto s = step();
    auto cell = cell_size();
    auto hy = grid_y_origin();
    auto htop = header_top();
    auto cltop = col_label_top();
    auto gx0 = grid_x();
    auto gy0 = grid_y();
    auto lfont = label_font();
    auto gridline = Color { 18, 18, 24 };

    // The clip decides how much work this paint does: the whole panel for a
    // full repaint, a few cells for the per-edit repaints. Skip everything
    // outside the clip so a click repaints only the damaged pixels instead
    // of re-running the whole editor (and re-encoding its pixels).
    auto hits = [&g](Rectangle const &r) {
      return g.hit_clip_rect(r);
    };

    // Only the content box is filled and painted; everything below it stays
    // untouched so the sixel image does not cover the whole terminal. When
    // the content scrolls it always fills the visible area, so paint the
    // whole panel then.
    auto fill_h = scroll_max() > 0 ? h : std::min(content_h(), h);
    fill(g, 0, 0, w, fill_h, background);

    // ---- header -----------------------------------------------------------
    // Three fixed lines above the column labels; they never scroll.
    if (hits({ 16, htop, w - 32, 3 * hline })) {
      g.set_font(Font { "Monospaced", HEADER_FONT, Font::PLAIN });
      g.set_foreground_color(text);
      char title[96];
      std::snprintf(title, sizeof title, "FONT EDITOR   glyph '%c' U+%04X", this->glyph >= 0x20 and this->glyph < 0x7F ? char(this->glyph) : '?', unsigned(this->glyph));
      g.draw_string(title, 16, htop);

      g.set_foreground_color(dim);
      g.draw_string("click: toggle  right: clear  c/r: clear  d/D: dump  q: quit  wheel: scroll", 16, htop + hline);

      g.set_foreground_color(this->is_changed() ? changed_color : dim);
      g.draw_string(this->is_changed() ? "modified (d to dump)" : "unmodified", 16, htop + 2 * hline);
    }

    // ---- column labels (0..15, fixed above the grid) -----------------------
    if (hits(column_label_rect())) {
      g.set_font(Font { "Monospaced", lfont, Font::PLAIN });
      g.set_foreground_color(dim);
      for (auto rx = 0; rx < GLYPH_W; ++rx) {
        char label[8];
        auto n = std::snprintf(label, sizeof label, "%d", rx);
        g.draw_string(label, gx0 + rx * s + (s - n * lfont) / 2, cltop);
      }
    }

    // ---- editor grid ------------------------------------------------------
    for (auto ry = 0; ry < GLYPH_H; ++ry) {
      for (auto rx = 0; rx < GLYPH_W; ++rx) {
        auto x = gx0 + rx * s;
        auto y = gy0 + ry * s;
        if (y < hy) {
          continue; // scrolled under the fixed header
        }
        if (not hits({ x, y, cell, cell })) {
          continue;
        }
        auto color = get_pixel(this->current(), rx, ry) ? ink : (ry > 22 ? descender_zone : empty);
        fill(g, x, y, cell, cell, color);
        // The right and bottom edges double as the grid lines that keep the
        // individual pixels apart now that the cells are adjacent.
        fill(g, x, y + cell - 1, cell, 1, gridline);
        fill(g, x + cell - 1, y, 1, cell, gridline);
      }
    }

    // Baseline: the bottom edge of row 22 (x-height bottom).
    if (gy0 + 23 * s - 1 >= hy) {
      fill(g, gx0 - 2, gy0 + 23 * s - 1, GLYPH_W * s + 4, 1, baseline_color);
    }

    // ---- row labels (0..31) and per-row hex --------------------------------
    // These scroll with the grid: each row's index sits in the left gutter and
    // the two bytes encoding the row sit immediately to the right of the grid.
    {
      auto *glyph_rows = this->current();
      g.set_font(Font { "Monospaced", lfont, Font::PLAIN });
      for (auto ry = 0; ry < GLYPH_H; ++ry) {
        auto y = gy0 + ry * s;
        if (y < hy) {
          continue; // scrolled under the fixed header
        }
        if (hits({ gx0 - row_label_gutter(), y, row_label_gutter(), s })) {
          char label[8];
          auto digits = ry >= 10 ? 2 : 1;
          std::snprintf(label, sizeof label, "%d", ry);
          g.set_foreground_color(dim);
          g.draw_string(label, gx0 - gutter() - digits * lfont, y);
        }
        if (hits({ per_row_hex_x(), y, per_row_hex_w(), s })) {
          char rowhex[8];
          std::snprintf(rowhex, sizeof rowhex, "%02X %02X", glyph_rows[ry * ROW_BYTES], glyph_rows[ry * ROW_BYTES + 1]);
          g.set_foreground_color(hex_color);
          g.draw_string(rowhex, per_row_hex_x(), y);
        }
      }
    }

    // ---- 4x and 1x previews ----------------------------------------------
    // Hidden while the content is scrolled: their top row would overlap the
    // fixed header (they scroll with the grid below it).
    if (gy0 >= hy and side_area_fits() and hits(preview_rect())) {
      auto dump_x = this->dump_x();
      g.set_font(Font { "Monospaced", HEX_FONT, Font::PLAIN });

      auto preview_x = dump_x;
      auto preview_y = gy0 + hexline;
      auto scale = 4;
      g.set_foreground_color(dim);
      g.draw_string("4x preview:", dump_x, gy0);
      fill(g, preview_x - 2, preview_y - 2, GLYPH_W * scale + 4, GLYPH_H * scale + 4, Color { 30, 30, 38 });
      this->blit_glyph_rows(g, this->current(), preview_x, preview_y, scale, ink);

      // The unscaled glyph sits after the whole "1x:" label: the label is
      // three HEX_FONT cells wide, not one, so the old +16 offset let the
      // glyph overlap the "x" and ":" of the label.
      auto label_x = preview_x + GLYPH_W * scale + 12;
      g.set_foreground_color(dim);
      g.draw_string("1x:", label_x, preview_y + GLYPH_H * scale - 8);
      this->blit_glyph_rows(g, this->current(), label_x + 3 * HEX_FONT + 8, preview_y + GLYPH_H * scale - 16, 1, ink);
    }

    // ---- overview strip of the printable glyphs ----------------------------
    auto per_row = this->per_row();
    auto strip_y = this->strip_y();
    for (auto code = 0x20; code < 0x7F; ++code) {
      auto i = code - 0x20;
      auto fx = 8 + (i % per_row) * GLYPH_W;
      auto fy = strip_y + (i / per_row) * GLYPH_H;
      if (fy < hy or not hits({ fx, fy, GLYPH_W, GLYPH_H })) {
        continue;
      }
      auto *grows = &this->font[code * GLYPH_H * ROW_BYTES];

      if (code == this->glyph) {
        fill(g, fx, fy, GLYPH_W, GLYPH_H, highlight);
        this->blit_glyph_rows(g, grows, fx, fy, 1, on_highlight);
      } else {
        auto *orig = &this->original[code * GLYPH_H * ROW_BYTES];
        auto changed = not std::equal(grows, grows + GLYPH_H * ROW_BYTES, orig);
        this->blit_glyph_rows(g, grows, fx, fy, 1, changed ? changed_color : dim);
      }
    }
  }
};

}

// Runs the interactive font editor on the graphic (sixel) backend. Lives at
// global scope like the other entry points declared in main.cpp.
void run_font_editor(bool bench, bool scrollbench) {
  terminal.set_title("tui++ font editor");
  terminal.set_type("sixel");

  auto frame = make_component<Frame>();
  frame->set_size(screen.get_size());
  // No window decoration: the editor paints its own background over the
  // content box only, so the border must not stroke the screen edges.
  frame->set_border(std::make_shared<EmptyBorder>(0, 0, 0, 0));

  auto panel = make_component<FontEditorPanel>();
  frame->add(panel);
  // The content pane would fill the whole window with its background below
  // the editor's content box; drop it so the initial image stops at the
  // content (the terminal's own background shows through below it).
  frame->get_content_pane()->set_opaque(false);

  // ---- menu bar -----------------------------------------------------------
  // The graphic screen measures components in pixels, so the menus are sized
  // from the screen's text metrics and styled to the editor's palette.
  auto menu_bg = Color { 24, 26, 34 };
  auto menu_text = Color { 200, 200, 205 };
  auto menu_bar = make_component<MenuBar>();
  menu_bar->set_background_color(menu_bg);
  menu_bar->set_foreground_color(menu_text);

  auto file_menu = make_component<Menu>("File");
  file_menu->set_mnemonic('F');
  auto edit_menu = make_component<Menu>("Edit");
  edit_menu->set_mnemonic('E');

  for (auto &&menu : { file_menu, edit_menu }) {
    menu->set_foreground_color(menu_text);
    menu->set_background_color(menu_bg);
    // The popup is created lazily inside Menu; style it now so its items
    // drop down in a column on the editor's dark background (the popup UI
    // installs the vertical layout and opacity).
    auto popup = menu->get_popup_menu();
    popup->set_background_color(Color { 30, 32, 40 });
  }

  auto make_item = [&](Menu &menu, std::string const &label, char mnemonic, std::function<void()> action) {
    auto item = make_component<MenuItem>(label, mnemonic);
    item->set_foreground_color(menu_text);
    item->add_listener([action, &menu](ActionEvent &e) {
      // The popup is toggled directly (not through the menu selection
      // manager), so an item click closes it explicitly before acting.
      menu.set_popup_menu_visible(false);
      action();
    });
    menu.add(item);
  };

  make_item(*file_menu, "Dump Glyph", 'd', [panel] {
    panel->dump_glyph();
  });
  make_item(*file_menu, "Dump Font", 'D', [panel] {
    panel->dump_font();
  });
  file_menu->add_separator();
  make_item(*file_menu, "Exit", 'q', [panel] {
    panel->quit();
  });

  make_item(*edit_menu, "Clear Glyph", 'c', [panel] {
    panel->clear_glyph();
    panel->repaint_after_glyph_edit(-1);
  });
  make_item(*edit_menu, "Reset Glyph", 'r', [panel] {
    panel->reset_glyph();
    panel->repaint_after_glyph_edit(-1);
  });

  menu_bar->add(file_menu);
  menu_bar->add(edit_menu);
  frame->set_menu_bar(menu_bar);

  // Clicking a top-level menu toggles its popup; opening one closes any
  // other open menu first (Swing's MenuSelectionManager behaviour), and
  // clicking anywhere else in the editor closes the open popup again.
  for (auto &&menu : { file_menu, edit_menu }) {
    menu->add_listener([menu, file_menu, edit_menu](MousePressEvent &e) {
      if (e.id == MousePressEvent::MOUSE_RELEASED) {
        auto open = not menu->is_popup_menu_visible();
        if (open) {
          for (auto &&other : { file_menu, edit_menu }) {
            if (other != menu) {
              other->set_popup_menu_visible(false);
            }
          }
        }
        menu->set_popup_menu_visible(open);
        e.consume();
      }
    });
  }

  // Mouse clicks toggle the grid pixels. The graphic screen converts the
  // terminal's cell-based mouse reports into the panel's pixel space, so the
  // coordinates match the editor geometry directly.
  panel->add_listener([panel, file_menu, edit_menu](MousePressEvent &e) {
    if (e.id != MousePressEvent::MOUSE_PRESSED) {
      return;
    }
    // A click outside the popup menu dismisses it.
    for (auto &&menu : { file_menu, edit_menu }) {
      if (menu->is_popup_menu_visible()) {
        menu->set_popup_menu_visible(false);
      }
    }
    auto gx = 0, gy = 0;
    if (not panel->hit_cell(e.x, e.y, gx, gy)) {
      return;
    }
    auto was_modified = panel->is_modified();
    if (e.button == MouseEvent::RIGHT_BUTTON) {
      panel->clear_cell(gx, gy);
    } else if (e.button == MouseEvent::LEFT_BUTTON) {
      panel->toggle_cell(gx, gy);
    } else {
      return;
    }
    e.consume();
    panel->repaint_after_cell_edit(gx, gy, was_modified);
  });

  // The mouse wheel scrolls the content on terminals too short to show the
  // whole editor at the mouse-aligned cell size (the 32-row grid alone needs
  // 32 terminal rows). Scroll by whole cells so the alignment survives.
  //
  // Scrolling moves every pixel of the content, so each step repaints the
  // whole scrolling area. The app produces that frame in a few milliseconds;
  // the terminal is the slow side (Windows Terminal decodes and re-renders
  // each sixel image before showing it). Wheel events arrive in bursts, so:
  //
  //   - on a terminal that keeps up (the last frame's write was fast), the
  //     repaints are rate-limited to ~30 fps for live feedback;
  //   - on a slow terminal, intermediate frames are dropped entirely: the
  //     scroll delta just accumulates, and one repaint lands on the final
  //     position once no wheel event has arrived for the quiet period. A
  //     burst can therefore never queue several seconds of catch-up frames.
  //
  // The quiet period is measured by a timer that re-posts itself as an event
  // after each short tick: every tick yields back to the event loop, so the
  // wheel events that arrive between the ticks are processed (and push the
  // deadline forward) before the timer is re-checked. The timer must not
  // sleep-wait inside one dispatch -- that would keep wheel events unread in
  // the terminal's input queue, so "quiet" could never be observed.
  struct ScrollState {
    int pending = 0;
    std::chrono::steady_clock::time_point last_frame { };
    std::chrono::steady_clock::time_point deadline { };
    bool trailing_posted = false;
  };
  auto scroll_state = std::make_shared<ScrollState>();

  auto flush_scroll = [panel, scroll_state] {
    if (scroll_state->pending == 0) {
      return;
    }
    if (panel->scroll_content(scroll_state->pending)) {
      panel->repaint_scrolled_area();
    }
    scroll_state->pending = 0;
    scroll_state->last_frame = std::chrono::steady_clock::now();
  };

  panel->add_listener([panel, scroll_state, flush_scroll](MouseWheelEvent &e) {
    auto ch = static_cast<SixelScreen&>(screen).get_cell_height();
    scroll_state->pending += e.wheel_rotation * ch;

    // The previous frame's write time reflects how long the terminal took to
    // accept it. A terminal that drains its input fast can follow live 30 fps
    // repaints; a slow one gets only the final position after the input goes
    // quiet, so the frames never pile up behind the wheel.
    auto write_ms = static_cast<SixelScreen&>(screen).get_last_write_ms();
    auto now = std::chrono::steady_clock::now();
    auto fast_terminal = write_ms <= 60.0;
    if (scroll_state->last_frame == std::chrono::steady_clock::time_point { } or (fast_terminal and now - scroll_state->last_frame >= std::chrono::milliseconds(33))) {
      flush_scroll();
    } else {
      // Debounce to the final position: the deadline moves forward with every
      // wheel event, and the timer below fires the repaint once it is reached.
      scroll_state->deadline = now + std::chrono::milliseconds(fast_terminal ? 50 : 200);
      if (not scroll_state->trailing_posted) {
        scroll_state->trailing_posted = true;
        // The timer holds the only strong reference to itself while it ticks
        // (one per scroll gesture, released when it fires), so it can re-post
        // its own next tick without an owner outside the event queue.
        auto timer = std::make_shared<std::function<void()>>();
        *timer = [scroll_state, flush_scroll, timer] {
          if (std::chrono::steady_clock::now() < scroll_state->deadline) {
            // Not quiet yet: yield to the event loop for a short tick so
            // queued wheel events are processed, then re-check.
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
            screen.post([timer] {
              (*timer)();
            });
            return;
          }
          scroll_state->trailing_posted = false;
          flush_scroll();
          // Break the self-reference from a follow-up invocation: dropping
          // the stored function right here would destroy it while it is
          // executing its own target.
          screen.post([timer] {
            *timer = { };
          });
        };
        screen.post([timer] {
          (*timer)();
        });
      }
    }
    e.consume();
  });

  frame->add_listener([panel](KeyEvent &e) {
    auto handled = true;
    if (e.id == KeyEvent::KEY_TYPED) {
      switch (e.get_key_char().get_code()) {
      case 'c':
        panel->clear_glyph();
        panel->repaint_after_glyph_edit(-1);
        break;
      case 'r':
        panel->reset_glyph();
        panel->repaint_after_glyph_edit(-1);
        break;
      case 'd':
        panel->dump_glyph();
        return;
      case 'D':
        panel->dump_font();
        return;
      case 'q':
        panel->quit();
        return;
      default:
        handled = false;
        break;
      }
    } else if (e.id == KeyEvent::KEY_PRESSED) {
      switch (e.get_key_code()) {
      case KeyEvent::VK_LEFT:
      case KeyEvent::VK_UP: {
        auto previous = panel->get_glyph();
        panel->prev_glyph();
        panel->repaint_after_glyph_edit(previous);
        break;
      }
      case KeyEvent::VK_RIGHT:
      case KeyEvent::VK_DOWN: {
        auto previous = panel->get_glyph();
        panel->next_glyph();
        panel->repaint_after_glyph_edit(previous);
        break;
      }
      default:
        handled = false;
        break;
      }
    } else {
      handled = false;
    }

    if (handled) {
      e.consume();
    }
  });

  frame->set_visible(true);

  if (bench) {
    // Benchmark mode: repaint the whole panel repeatedly, alternating between
    // two scroll positions so every frame's content actually changes, and
    // report the per-frame cost. Runs with stdout redirected so the sixel
    // bytes and the timings land in the capture file.
    auto &gs = static_cast<SixelScreen&>(screen);
    auto ch = gs.get_cell_height();
    auto origin = panel->get_location_on_screen();
    auto const frames = 30;
    auto total_ms = 0.0;
    std::fprintf(stderr, "bench: %dx%d px, cell %dx%d, frames %d\n", gs.get_pixel_width(), gs.get_pixel_height(), gs.get_cell_width(), gs.get_cell_height(), frames);
    for (auto i = 0; i < frames; ++i) {
      panel->scroll_content((i % 2 == 0 ? 1 : -1) * ch);
      auto t0 = std::chrono::steady_clock::now();
      screen.repaint_region({ origin.x, origin.y, panel->get_width(), panel->get_height() });
      auto t1 = std::chrono::steady_clock::now();
      auto ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
      total_ms += ms;
      std::fprintf(stderr, "frame %2d: %.1f ms (encode %.1f ms, write %.1f ms, sixel %zu bytes)\n", i, ms, gs.get_last_encode_ms(), gs.get_last_write_ms(), gs.get_last_bytes());
    }
    std::fprintf(stderr, "bench: average %.1f ms/frame (%.1f fps)\n", total_ms / frames, frames / total_ms * 1000.0);
    std::exit(0);
  }

  if (scrollbench) {
    // Scroll-benchmark mode: feed the wheel handler a burst of synthetic
    // wheel events (like a real fast scroll) and check that the scroll
    // coalescing neither hangs nor repaints once per event. A burst should
    // produce one immediate frame plus one trailing frame at the final
    // position, so the flush count must stay tiny.
    auto &gs = static_cast<SixelScreen&>(screen);
    std::fprintf(stderr, "scrollbench: %dx%d px, cell %dx%d, flushes before: %zu\n", gs.get_pixel_width(), gs.get_pixel_height(), gs.get_cell_width(), gs.get_cell_height(), gs.get_flush_count());
    screen.post([panel, gs = &gs] {
      auto const events = 20;
      auto t0 = std::chrono::steady_clock::now();
      for (auto i = 0; i < events; ++i) {
        // Synthetic wheel event in panel coordinates; the handler does not
        // use the position, only the rotation.
        MouseWheelEvent e { panel, InputEvent::NO_MODIFIERS, 0, 0, i % 2 == 0 ? 1 : -1 };
        panel->dispatch_event(e);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      auto t1 = std::chrono::steady_clock::now();
      std::fprintf(stderr, "scrollbench: dispatched %d wheel events in %.0f ms\n", events, std::chrono::duration<double, std::milli>(t1 - t0).count());
      screen.post([panel, gs] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::fprintf(stderr, "scrollbench: flushes after: %zu (initial draw + scroll frames; a burst must add at most 2)\n", gs->get_flush_count());
        std::exit(0);
      });
    });
    terminal.run_event_loop();
  }

  terminal.run_event_loop();
}
