#include <tui++/terminal/sixel/SixelGraphics.h>
#include <tui++/terminal/sixel/SixelScreen.h>
#include <tui++/terminal/sixel/Font16x32.h>

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/util/utf-8.h>
#include <tui++/util/unicode.h>
#include <tui++/util/log.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

namespace tui {

namespace {

// Rendered for code points the bitmap font does not cover (U+0080 and above):
// a 16x32 box, matching the raster's cell.
constexpr uint8_t MISSING_GLYPH[detail::FONT_HEIGHT * 2] = {
    0xFF, 0xFF, 0xFF, 0xFF, // top edge (2 px)
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, // left / right edges
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03, 0xC0, 0x03,
    0xFF, 0xFF, 0xFF, 0xFF  // bottom edge (2 px)
};

// "#RRGGBB", the format the log lines use for colors.
std::string color_hex(Color const &color) {
  char buffer[8];
  std::snprintf(buffer, sizeof buffer, "#%02X%02X%02X", int(color.red()), int(color.green()), int(color.blue()));
  return buffer;
}

}

SixelGraphics::SixelGraphics(SixelScreen &screen) :
    SixelGraphics(screen, { 0, 0, screen.get_width(), screen.get_height() }, 0, 0) {
}

SixelGraphics::SixelGraphics(SixelScreen &screen, const Rectangle &clip_rect, int dx, int dy) :
    screen(screen), dx(dx), dy(dy), clip(clip_rect),
    font(laf::LookAndFeel::get<Font>("defaultFont", Font { })) {
}

std::optional<Color> SixelGraphics::get_fill_color() const {
  return this->background_color ? this->background_color : this->foreground_color;
}

std::optional<Color> SixelGraphics::get_line_color() const {
  return this->foreground_color ? this->foreground_color : this->background_color;
}

// Returns the pixel rectangle (offset by dx/dy) clipped to the clip rect.
Rectangle SixelGraphics::clipped(int x, int y, int width, int height) const {
  return Rectangle { x + this->dx, y + this->dy, width, height } & this->clip;
}

void SixelGraphics::draw_pixel_rect(int x, int y, int width, int height) {
  if (auto rect = clipped(x, y, width, height); not rect.empty()) {
    if (auto color = get_line_color()) {
      log_graphics_ln("SixelGraphics::draw_pixel_rect (" << rect.x << ", " << rect.y << " " << rect.width << 'x' << rect.height << ") color=" << color_hex(*color));
      this->screen.fill_pixels(rect, *color);
    }
  }
}

int SixelGraphics::stroke_width() const {
  switch (this->stroke) {
  case Stroke::LIGHT:
  case Stroke::DASHED:
    return 1;
  case Stroke::HEAVY:
  case Stroke::DOUBLE:
  case Stroke::BOLD:
    return 2;
  }
  return 1;
}

void SixelGraphics::clip_rect(int x, int y, int width, int height) {
  this->clip = Rectangle { x + this->dx, y + this->dy, width, height } & this->clip;
}

std::unique_ptr<Graphics> SixelGraphics::create() {
  return std::make_unique<SixelGraphics>(this->screen, this->clip, this->dx, this->dy);
}

std::unique_ptr<Graphics> SixelGraphics::create(int x, int y, int width, int height) {
  auto g = create();
  g->translate(x, y);
  g->clip_rect(0, 0, width, height);
  return g;
}

void SixelGraphics::draw_char(const Char &c, int x, int y, std::optional<Attributes> const &attributes) {
  log_graphics_ln("SixelGraphics::draw_char U+" << std::hex << std::uppercase << uint32_t(c.get_code()) << std::nouppercase << std::dec << " at (" << x << ", " << y << ")");
  blit_glyph(c.get_code(), x, y);
}

void SixelGraphics::blit_glyph(char32_t code, int x, int y) {
  auto const *glyph = (code < 128) ? detail::FONT16X32_BASIC[code] : MISSING_GLYPH;
  auto px = x + this->dx;
  auto py = y + this->dy;

  // The font size is the glyph width in pixels; the 16x32 raster is twice as
  // tall as it is wide.
  auto width = std::max(this->font.get_size(), 1);
  auto height = 2 * width;

  if (not this->clip.intersects(px, py, width, height)) {
    return;
  }

  // BOLD doubles every stroke by mirroring each set pixel one column to the
  // right, at the 16x32 raster resolution.
  auto bold = bool(this->font.get_style() & Font::BOLD);
  auto styled = std::array<uint16_t, detail::FONT_HEIGHT> { };
  for (auto sy = 0; sy < detail::FONT_HEIGHT; ++sy) {
    auto row = uint16_t((glyph[sy * 2] << 8) | glyph[sy * 2 + 1]);
    styled[sy] = bold ? uint16_t(row | (row >> 1)) : row;
  }

  // Scale the raster to the requested size (nearest-neighbor); ITALIC shears
  // the rows, shifting the top rows right so the glyph leans forward.
  auto italic = bool(this->font.get_style() & Font::ITALIC);
  auto shear_max = italic ? std::max(1, width / 4) : 0;
  auto row_bytes = (width + 7) / 8;
  auto scaled = std::vector<uint8_t>(std::size_t(row_bytes) * height, 0);

  for (auto row_y = 0; row_y < height; ++row_y) {
    auto sy = row_y * detail::FONT_HEIGHT / height;
    auto shear = italic ? (height - 1 - row_y) * shear_max / (height - 1) : 0;
    auto row = styled[sy];
    for (auto col_x = 0; col_x < width; ++col_x) {
      // The sheared column is checked before scaling: division truncates
      // toward zero, so a negative column must not wrap around to column 0.
      auto shifted = col_x - shear;
      if (shifted < 0) {
        continue;
      }
      auto sx = shifted * detail::FONT_WIDTH / width;
      if (row & (0x8000 >> sx)) {
        scaled[std::size_t(row_y) * row_bytes + col_x / 8] |= uint8_t(0x80 >> (col_x % 8));
      }
    }
  }

  // An unset foreground means "the terminal's default text color", which the
  // text screen renders as the terminal default (white); mirror that here. A
  // null background leaves the pixels under the glyph untouched.
  log_graphics_ln("SixelGraphics::blit_glyph U+" << std::hex << std::uppercase << uint32_t(code) << std::nouppercase << std::dec << " at (" << px << ", " << py << " " << width << 'x' << height << ") fg=" << color_hex(this->foreground_color.value_or(WHITE_COLOR)));
  this->screen.blit_glyph(px, py, scaled.data(), width, height, this->foreground_color.value_or(WHITE_COLOR), this->background_color);
}

void SixelGraphics::draw_hline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  log_graphics_ln("SixelGraphics::draw_hline at (" << x << ", " << y << ") length " << length);
  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_rect(x, y - 1, length, 1);
    draw_pixel_rect(x, y + 1, length, 1);
  } else {
    auto line_w = stroke_width();
    draw_pixel_rect(x, y - line_w / 2, length, line_w);
  }
}

void SixelGraphics::draw_vline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  log_graphics_ln("SixelGraphics::draw_vline at (" << x << ", " << y << ") length " << length);
  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_rect(x - 1, y, 1, length);
    draw_pixel_rect(x + 1, y, 1, length);
  } else {
    auto line_w = stroke_width();
    draw_pixel_rect(x - line_w / 2, y, line_w, length);
  }
}

void SixelGraphics::draw_rect(int x, int y, int width, int height) {
  if (width <= 0 or height <= 0) {
    return;
  }
  if (auto color = get_line_color()) {
    // The stroke follows the edges of the rect that was asked for (offset by
    // the graphics translation), with every edge band clipped to the current
    // clip. Stroking the border of the *clipped* rect instead -- as drawing
    // bands around the intersection would -- traces the clip's own perimeter
    // whenever a rect only partially covers the damaged region: a repaint of
    // a menu item would repaint the frame border that crosses it as a cyan
    // frame around the item.
    auto left = x + this->dx;
    auto top = y + this->dy;
    auto right = left + width;
    auto bottom = top + height;
    auto line_w = std::min(stroke_width(), std::min(width, height));

    log_graphics_ln("SixelGraphics::draw_rect (" << left << ", " << top << " " << width << 'x' << height << ") color=" << color_hex(*color));

    this->screen.fill_pixels(Rectangle { left, top, width, line_w } & this->clip, *color); // top
    this->screen.fill_pixels(Rectangle { left, bottom - line_w, width, line_w } & this->clip, *color); // bottom
    this->screen.fill_pixels(Rectangle { left, top, line_w, height } & this->clip, *color); // left
    this->screen.fill_pixels(Rectangle { right - line_w, top, line_w, height } & this->clip, *color); // right

    if (this->stroke == Stroke::DOUBLE) {
      auto inset = 2;
      if (width > 2 * inset + 2 and height > 2 * inset + 2) {
        this->screen.fill_pixels(Rectangle { left + inset, top + inset, width - 2 * inset, 1 } & this->clip, *color);
        this->screen.fill_pixels(Rectangle { left + inset, bottom - inset - 1, width - 2 * inset, 1 } & this->clip, *color);
        this->screen.fill_pixels(Rectangle { left + inset, top + inset, 1, height - 2 * inset } & this->clip, *color);
        this->screen.fill_pixels(Rectangle { right - inset - 1, top + inset, 1, height - 2 * inset } & this->clip, *color);
      }
    }
  }
}

void SixelGraphics::draw_rounded_rect(int x, int y, int width, int height) {
  // Rounding the corners is not implemented yet; fall back to a rectangle.
  log_graphics_ln("SixelGraphics::draw_rounded_rect (" << x << ", " << y << " " << width << 'x' << height << ")");
  draw_rect(x, y, width, height);
}

void SixelGraphics::draw_string(const std::string &str, int x, int y, std::optional<Attributes> const &attributes) {
  log_graphics_ln("SixelGraphics::draw_string \"" << str << "\" at (" << x << ", " << y << ")");
  auto cx = x;
  auto index = std::size_t { 0 };
  while (index < str.size()) {
    auto code = char32_t { };
    auto len = util::mb_to_c32(str.data() + index, str.size() - index, &code);
    if (len <= 0) {
      index += 1;
      continue;
    }
    draw_char(Char(code), cx, y, attributes);
    cx += util::unicode::glyph_width(code) * this->font.get_size();
    index += std::size_t(len);
  }
}

void SixelGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto rect = clipped(x, y, width, height); not rect.empty()) {
    if (auto color = get_fill_color()) {
      log_graphics_ln("SixelGraphics::fill_rect (" << rect.x << ", " << rect.y << " " << rect.width << 'x' << rect.height << ") color=" << color_hex(*color));
      this->screen.fill_pixels(rect, *color);
    }
  }
}

Rectangle SixelGraphics::get_clip_rect() const {
  return { this->clip.x - this->dx, this->clip.y - this->dy, this->clip.width, this->clip.height };
}

void SixelGraphics::set_clip_rect(const Rectangle &rect) {
  this->clip = { rect.x + this->dx, rect.y + this->dy, rect.width, rect.height };
}

bool SixelGraphics::hit_clip_rect(int x, int y, int width, int height) const {
  return this->clip.intersects(x + this->dx, y + this->dy, width, height);
}

std::optional<Color> const& SixelGraphics::get_foreground_color() const {
  return this->foreground_color;
}

void SixelGraphics::set_foreground_color(std::optional<Color> const &color) {
  this->foreground_color = color;
}

std::optional<Color> const& SixelGraphics::get_background_color() const {
  return this->background_color;
}

void SixelGraphics::set_background_color(std::optional<Color> const &color) {
  this->background_color = color;
}

Font SixelGraphics::get_font() const {
  return this->font;
}

void SixelGraphics::set_font(const Font &font) {
  this->font = font;
}

Stroke SixelGraphics::get_stroke() const {
  return this->stroke;
}

void SixelGraphics::set_stroke(Stroke stroke) {
  this->stroke = stroke;
}

void SixelGraphics::translate(int dx, int dy) {
  this->dx += dx;
  this->dy += dy;
}

void SixelGraphics::flush() {
  this->screen.flush();
}

}
