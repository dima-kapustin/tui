#include <tui++/sixel/SixelGraphics.h>
#include <tui++/sixel/SixelScreen.h>
#include <tui++/sixel/Font8x8.h>

#include <tui++/util/utf-8.h>

#include <algorithm>

namespace tui {

namespace {

// Rendered for code points the bitmap font does not cover (U+0080 and above).
constexpr uint8_t MISSING_GLYPH[8] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF };

}

SixelGraphics::SixelGraphics(SixelScreen &screen) :
    SixelGraphics(screen, { 0, 0, screen.get_width(), screen.get_height() }, 0, 0) {
}

SixelGraphics::SixelGraphics(SixelScreen &screen, const Rectangle &clip_rect, int dx, int dy) :
    screen(screen), dx(dx), dy(dy), clip(clip_rect) {
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
  blit_glyph(c.get_code(), x, y);
}

void SixelGraphics::blit_glyph(char32_t code, int x, int y) {
  auto const *glyph = (code < 128) ? detail::FONT8X8_BASIC[code] : MISSING_GLYPH;
  auto px = x + this->dx;
  auto py = y + this->dy;
  if (this->clip.intersects(px, py, detail::FONT_WIDTH, detail::FONT_HEIGHT)) {
    this->screen.blit_glyph(px, py, glyph, this->foreground_color, this->background_color);
  }
}

void SixelGraphics::draw_hline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_rect(x, y - 1, length, 1);
    draw_pixel_rect(x, y + 1, length, 1);
  } else {
    auto line_w = stroke_width();
    draw_pixel_rect(x, y - line_w / 2, length, line_w);
  }
}

void SixelGraphics::draw_vline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_rect(x - 1, y, 1, length);
    draw_pixel_rect(x + 1, y, 1, length);
  } else {
    auto line_w = stroke_width();
    draw_pixel_rect(x - line_w / 2, y, line_w, length);
  }
}

void SixelGraphics::draw_rect(int x, int y, int width, int height) {
  auto rect = clipped(x, y, width, height);
  if (rect.empty()) {
    return;
  }
  if (auto color = get_line_color()) {
    auto line_w = std::min(stroke_width(), std::min(rect.width, rect.height));

    this->screen.fill_pixels({ rect.x, rect.y, rect.width, line_w }, *color); // top
    this->screen.fill_pixels({ rect.x, rect.bottom() - line_w, rect.width, line_w }, *color); // bottom
    this->screen.fill_pixels({ rect.x, rect.y, line_w, rect.height }, *color); // left
    this->screen.fill_pixels({ rect.right() - line_w, rect.y, line_w, rect.height }, *color); // right

    if (this->stroke == Stroke::DOUBLE) {
      auto inset = 2;
      if (rect.width > 2 * inset + 2 and rect.height > 2 * inset + 2) {
        this->screen.fill_pixels({ rect.x + inset, rect.y + inset, rect.width - 2 * inset, 1 }, *color);
        this->screen.fill_pixels({ rect.x + inset, rect.bottom() - inset - 1, rect.width - 2 * inset, 1 }, *color);
        this->screen.fill_pixels({ rect.x + inset, rect.y + inset, 1, rect.height - 2 * inset }, *color);
        this->screen.fill_pixels({ rect.right() - inset - 1, rect.y + inset, 1, rect.height - 2 * inset }, *color);
      }
    }
  }
}

void SixelGraphics::draw_rounded_rect(int x, int y, int width, int height) {
  // Rounding the corners is not implemented yet; fall back to a rectangle.
  draw_rect(x, y, width, height);
}

void SixelGraphics::draw_string(const std::string &str, int x, int y, std::optional<Attributes> const &attributes) {
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
    cx += detail::FONT_WIDTH;
    index += std::size_t(len);
  }
}

void SixelGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto rect = clipped(x, y, width, height); not rect.empty()) {
    if (auto color = get_fill_color()) {
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
