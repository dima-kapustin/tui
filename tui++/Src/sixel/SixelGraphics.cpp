#include <tui++/sixel/SixelGraphics.h>
#include <tui++/sixel/SixelScreen.h>

#include <algorithm>

namespace tui {

SixelGraphics::SixelGraphics(SixelScreen &screen) :
    SixelGraphics(screen, { 0, 0, screen.get_width() * SixelScreen::CELL_WIDTH, screen.get_height() * SixelScreen::CELL_HEIGHT }, 0, 0) {
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

Rectangle SixelGraphics::to_pixels(int x, int y, int width, int height) const {
  auto rect = Rectangle { //
    (x + this->dx) * SixelScreen::CELL_WIDTH, //
    (y + this->dy) * SixelScreen::CELL_HEIGHT, //
    width * SixelScreen::CELL_WIDTH, //
    height * SixelScreen::CELL_HEIGHT };
  return rect & this->clip;
}

void SixelGraphics::fill_cells(int x, int y, int width, int height) {
  if (not this->background_color) {
    return;
  }
  if (auto rect = to_pixels(x, y, width, height); not rect.empty()) {
    this->screen.fill_pixels(rect, *this->background_color);
  }
}

void SixelGraphics::draw_pixel_line(int x, int y, int width, int height) {
  if (auto rect = this->clip.intersection(x, y, width, height); not rect.empty()) {
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
  auto left = (x + this->dx) * SixelScreen::CELL_WIDTH;
  auto top = (y + this->dy) * SixelScreen::CELL_HEIGHT;
  auto right = left + width * SixelScreen::CELL_WIDTH;
  auto bottom = top + height * SixelScreen::CELL_HEIGHT;

  auto clip_left = std::max(this->clip.x, left);
  auto clip_right = std::min(clip_left + this->clip.width, right);
  auto clip_top = std::max(this->clip.y, top);
  auto clip_bottom = std::min(clip_top + this->clip.height, bottom);

  this->clip.set(clip_left, clip_top, clip_right - clip_left, clip_bottom - clip_top);
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
  // Text rendering requires a font rasterizer, which is not implemented yet.
}

void SixelGraphics::draw_hline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  auto px = x + this->dx;
  auto py = y + this->dy;
  auto cy = py * SixelScreen::CELL_HEIGHT + SixelScreen::CELL_HEIGHT / 2;
  auto w = length * SixelScreen::CELL_WIDTH;

  fill_cells(px, py, length, 1);

  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_line(px * SixelScreen::CELL_WIDTH, cy - 1, w, 1);
    draw_pixel_line(px * SixelScreen::CELL_WIDTH, cy + 1, w, 1);
  } else {
    auto line_w = stroke_width();
    draw_pixel_line(px * SixelScreen::CELL_WIDTH, cy - line_w / 2, w, line_w);
  }
}

void SixelGraphics::draw_vline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  auto px = x + this->dx;
  auto py = y + this->dy;
  auto cx = px * SixelScreen::CELL_WIDTH + SixelScreen::CELL_WIDTH / 2;
  auto h = length * SixelScreen::CELL_HEIGHT;

  fill_cells(px, py, 1, length);

  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_line(cx - 1, py * SixelScreen::CELL_HEIGHT, 1, h);
    draw_pixel_line(cx + 1, py * SixelScreen::CELL_HEIGHT, 1, h);
  } else {
    auto line_w = stroke_width();
    draw_pixel_line(cx - line_w / 2, py * SixelScreen::CELL_HEIGHT, line_w, h);
  }
}

void SixelGraphics::draw_rect(int x, int y, int width, int height) {
  auto px = x + this->dx;
  auto py = y + this->dy;
  auto left = px * SixelScreen::CELL_WIDTH;
  auto top = py * SixelScreen::CELL_HEIGHT;
  auto right = left + width * SixelScreen::CELL_WIDTH;
  auto bottom = top + height * SixelScreen::CELL_HEIGHT;

  auto line_w = stroke_width();

  // The border occupies the outer cell ring; fill its background first.
  fill_cells(px, py, width, 1);
  fill_cells(px, py + height - 1, width, 1);
  fill_cells(px, py, 1, height);
  fill_cells(px + width - 1, py, 1, height);

  draw_pixel_line(left, top, right - left, line_w);
  draw_pixel_line(left, bottom - line_w, right - left, line_w);
  draw_pixel_line(left, top + line_w, line_w, bottom - top - 2 * line_w);
  draw_pixel_line(right - line_w, top + line_w, line_w, bottom - top - 2 * line_w);

  if (this->stroke == Stroke::DOUBLE) {
    draw_pixel_line(left + 2, top + 2, right - left - 4, 1);
    draw_pixel_line(left + 2, bottom - 3, right - left - 4, 1);
    draw_pixel_line(left + 2, top + 2, 1, bottom - top - 4);
    draw_pixel_line(right - 3, top + 2, 1, bottom - top - 4);
  }
}

void SixelGraphics::draw_rounded_rect(int x, int y, int width, int height) {
  // The sixel backend does not round the corners yet.
  draw_rect(x, y, width, height);
}

void SixelGraphics::draw_string(const std::string &str, int x, int y, std::optional<Attributes> const &attributes) {
  // Text rendering requires a font rasterizer, which is not implemented yet.
}

void SixelGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto rect = to_pixels(x, y, width, height); not rect.empty()) {
    if (auto color = get_fill_color()) {
      this->screen.fill_pixels(rect, *color);
    }
  }
}

Rectangle SixelGraphics::get_clip_rect() const {
  return { //
    (this->clip.x - this->dx * SixelScreen::CELL_WIDTH) / SixelScreen::CELL_WIDTH, //
    (this->clip.y - this->dy * SixelScreen::CELL_HEIGHT) / SixelScreen::CELL_HEIGHT, //
    this->clip.width / SixelScreen::CELL_WIDTH, //
    this->clip.height / SixelScreen::CELL_HEIGHT };
}

void SixelGraphics::set_clip_rect(const Rectangle &rect) {
  this->clip = { //
    (rect.x + this->dx) * SixelScreen::CELL_WIDTH, //
    (rect.y + this->dy) * SixelScreen::CELL_HEIGHT, //
    rect.width * SixelScreen::CELL_WIDTH, //
    rect.height * SixelScreen::CELL_HEIGHT };
}

bool SixelGraphics::hit_clip_rect(int x, int y, int width, int height) const {
  return this->clip.intersects( //
      (x + this->dx) * SixelScreen::CELL_WIDTH, //
      (y + this->dy) * SixelScreen::CELL_HEIGHT, //
      width * SixelScreen::CELL_WIDTH, //
      height * SixelScreen::CELL_HEIGHT);
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
