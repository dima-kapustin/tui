#include <tui++/Symbols.h>
#include <tui++/CharIterator.h>

#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

namespace tui {

TerminalGraphics::BoxCharacters TerminalGraphics::LIGHT_BOX = //
    { .top_left = Symbols::DOWN_AND_RIGHT_LIGHT, //
      .top = Symbols::HORIZONTAL_LIGHT, //
      .top_right = Symbols::DOWN_AND_LEFT_LIGHT, //
      .right = Symbols::VERTICAL_LIGHT, //
      .bottom_right = Symbols::UP_AND_LEFT_LIGHT, //
      .bottom = Symbols::HORIZONTAL_LIGHT, //
      .bottom_left = Symbols::UP_AND_RIGHT_LIGHT, //
      .left = Symbols::VERTICAL_LIGHT, //
      .horizontal = Symbols::HORIZONTAL_LIGHT, //
      .vertical = Symbols::VERTICAL_LIGHT };

TerminalGraphics::BoxCharacters TerminalGraphics::HEAVY_BOX = //
    { .top_left = Symbols::DOWN_AND_RIGHT_HEAVY, //
      .top = Symbols::HORIZONTAL_HEAVY, //
      .top_right = Symbols::DOWN_AND_LEFT_HEAVY, //
      .right = Symbols::VERTICAL_HEAVY, //
      .bottom_right = Symbols::UP_AND_LEFT_HEAVY, //
      .bottom = Symbols::HORIZONTAL_HEAVY, //
      .bottom_left = Symbols::UP_AND_RIGHT_HEAVY, //
      .left = Symbols::VERTICAL_HEAVY, //
      .horizontal = Symbols::HORIZONTAL_HEAVY, //
      .vertical = Symbols::VERTICAL_HEAVY };

TerminalGraphics::BoxCharacters TerminalGraphics::DOUBLE_BOX = //
    { .top_left = Symbols::DOWN_AND_RIGHT_DOUBLE, //
      .top = Symbols::HORIZONTAL_DOUBLE, //
      .top_right = Symbols::DOWN_AND_LEFT_DOUBLE, //
      .right = Symbols::VERTICAL_DOUBLE, //
      .bottom_right = Symbols::UP_AND_LEFT_DOUBLE, //
      .bottom = Symbols::HORIZONTAL_DOUBLE, //
      .bottom_left = Symbols::UP_AND_RIGHT_DOUBLE, //
      .left = Symbols::VERTICAL_DOUBLE, //
      .horizontal = Symbols::HORIZONTAL_DOUBLE, //
      .vertical = Symbols::VERTICAL_DOUBLE };

TerminalGraphics::BoxCharacters TerminalGraphics::ROUNDED_LIGHT_BOX = //
    { .top_left = Symbols::DOWN_AND_RIGHT_ARC, //
      .top = Symbols::HORIZONTAL_LIGHT, //
      .top_right = Symbols::DOWN_AND_LEFT_ARC, //
      .right = Symbols::VERTICAL_LIGHT, //
      .bottom_right = Symbols::UP_AND_LEFT_ARC, //
      .bottom = Symbols::HORIZONTAL_LIGHT, //
      .bottom_left = Symbols::UP_AND_RIGHT_ARC, //
      .left = Symbols::VERTICAL_LIGHT, //
      .horizontal = Symbols::HORIZONTAL_LIGHT, //
      .vertical = Symbols::VERTICAL_LIGHT };

constexpr std::optional<Attributes> operator|(std::optional<Attributes> const &x, std::optional<Attributes> const &y) {
  return x ? (y ? (x.value() | y.value()) : x) : std::nullopt;
}

TerminalGraphics::TerminalGraphics(TerminalScreen &screen) :
    TerminalGraphics(screen, { 0, 0, screen.get_width(), screen.get_height() }, 0, 0) {
}

TerminalGraphics::TerminalGraphics(TerminalScreen &screen, const Rectangle &clip_rect, int dx, int dy) :
    screen(screen), dx(dx), dy(dy), clip(clip_rect) {
}

const TerminalGraphics::BoxCharacters& TerminalGraphics::get_box_chars(Stroke stroke) {
  switch (stroke) {
  default:
  case Stroke::LIGHT:
    return LIGHT_BOX;
  case Stroke::HEAVY:
    return HEAVY_BOX;
  case Stroke::DOUBLE:
    return DOUBLE_BOX;
  }
}

void TerminalGraphics::draw_rect(int x, int y, int width, int height, const BoxCharacters &chars) {
  int left = x + this->dx, top = y + this->dy;
  int right = left + width, bottom = top + height;
  int clip_left = this->clip.x;
  int clip_top = this->clip.y;
  int clip_right = clip_left + this->clip.width;
  int clip_bottom = clip_top + this->clip.height;

  int max_left = std::max(left, clip_left);
  int min_right = std::min(right, clip_right);
  int max_top = std::max(top, clip_top);
  int min_bottom = std::min(bottom, clip_bottom);

  // If the top of the box is outside the clipping rectangle, don't bother to draw the clipTop.
  if (top >= clip_top and top < clip_bottom) {
    if (left == max_left) {
      // top left corner
      this->screen.draw_char(chars.top_left, left, top, this->foreground_color, this->background_color, this->attributes);
    }
    for (int i = max_left + 1; i < min_right - 1; i++) {
      // top horizontal line
      this->screen.draw_char(chars.top, i, top, this->foreground_color, this->background_color, this->attributes);
    }
    if (right == min_right) {
      // top right corner
      this->screen.draw_char(chars.top_right, right - 1, top, this->foreground_color, this->background_color, this->attributes);
    }
  }

  // If the bottom of the box is outside the clipping rectangle, don't bother
  if (bottom >= clip_top and bottom <= clip_bottom) {
    if (left == max_left) {
      // bottom left corner
      this->screen.draw_char(chars.bottom_left, left, bottom - 1, this->foreground_color, this->background_color, this->attributes);
    }
    for (int i = max_left + 1; i < min_right - 1; i++) {
      // bottom horizontal line
      this->screen.draw_char(chars.bottom, i, bottom - 1, this->foreground_color, this->background_color, this->attributes);
    }
    if (right == min_right) {
      // bottom right corner
      this->screen.draw_char(chars.bottom_right, right - 1, bottom - 1, this->foreground_color, this->background_color, this->attributes);
    }
  }

  // If the left side of the box is outside the clipping rectangle, don't bother.
  if (left >= clip_left and left < clip_right) {
    for (int i = max_top + 1; i < min_bottom - 1; i++) {
      this->screen.draw_char(chars.left, left, i, this->foreground_color, this->background_color, this->attributes);
    }
  }
  //
  // If the right side of the box is outside the clipping rectangle, don't bother.
  if (right >= clip_left and right <= clip_right) {
    for (int i = max_top + 1; i < min_bottom - 1; i++) {
      this->screen.draw_char(chars.right, right - 1, i, this->foreground_color, this->background_color, this->attributes);
    }
  }
}

void TerminalGraphics::clip_rect(int x, int y, int width, int height) {
  int left = x + this->dx;
  int top = y + this->dy;
  int right = left + width;
  int bottom = top + height;

  int clip_left = std::max(this->clip.x, left);
  int clip_right = std::min(clip_left + this->clip.width, right);
  int clip_top = std::max(this->clip.y, top);
  int clip_bottom = std::min(clip_top + this->clip.height, bottom);

  int clip_width = clip_right - clip_left;
  int clip_height = clip_bottom - clip_top;

  this->clip.set(clip_left, clip_top, clip_width, clip_height);
}

std::unique_ptr<Graphics> TerminalGraphics::create() {
  return std::make_unique<TerminalGraphics>(this->screen, this->clip, this->dx, this->dx);
}

std::unique_ptr<Graphics> TerminalGraphics::create(int x, int y, int width, int height) {
  auto g = create();
  g->translate(x, y);
  g->clip_rect(0, 0, width, height);
  return g;
}

void TerminalGraphics::draw_char(const Char &c, int x, int y, std::optional<Attributes> const &attributes) {
  if (this->clip.contains(x + this->dx, y + this->dy)) {
    this->screen.draw_char(c, x + this->dx, y + this->dy, this->foreground_color, this->background_color, this->attributes | attributes);
  }
}

void TerminalGraphics::draw_hline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  if (auto const rect = this->clip.intersection(x + this->dx, y + this->dy, length, 1)) {
    auto &chars = get_box_chars(this->stroke);
    for (int x = rect.left(), right = rect.right(); x < right; ++x) {
      this->screen.draw_char(chars.horizontal, x, rect.y, this->foreground_color, this->background_color, this->attributes | attributes);
    }
  }
}

void TerminalGraphics::draw_rect(int x, int y, int width, int height) {
  draw_rect(x, y, width, height, get_box_chars(this->stroke));
}

void TerminalGraphics::draw_rounded_rect(int x, int y, int width, int height) {
  draw_rect(x, y, width, height, ROUNDED_LIGHT_BOX);
}

void TerminalGraphics::draw_string(const std::string &str, int x, int y, std::optional<Attributes> const &attributes) {
  x += this->dx;
  y += this->dy;
  if (auto const rect = this->clip.intersection(x, y, util::glyph_width(str), 1)) {
    auto const left = rect.left(), right = rect.right();
    for (auto &&ch : to_chars(str)) {
      auto glyph_width = ch.glyph_width();
      if ((x + glyph_width) >= left) {
        this->screen.draw_char(ch, x, y, this->foreground_color, this->background_color, this->attributes | attributes);
      }
      x += glyph_width;
      if (x >= right) {
        break;
      }
    }
  }
}

void TerminalGraphics::draw_vline(int x, int y, int length, std::optional<Attributes> const &attributes) {
  if (auto const rect = this->clip.intersection(x + this->dx, y + this->dy, 1, length)) {
    auto &chars = get_box_chars(this->stroke);
    for (auto y = rect.top(), bottom = rect.bottom(); y < bottom; ++y) {
      this->screen.draw_char(chars.vertical, rect.x, y, this->foreground_color, this->background_color, this->attributes | attributes);
    }
  }
}

void TerminalGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto const rect = this->clip.intersection(x + this->dx, y + this->dy, width, height)) {
    for (auto j = rect.top(), bottom = rect.bottom(); j < bottom; j++) {
      for (auto i = rect.left(), right = rect.right(); i < right; i++) {
        this->screen.draw_char(' ', i, j, this->foreground_color, this->background_color, this->attributes);
      }
    }
  }
}

Rectangle TerminalGraphics::get_clip_rect() const {
  auto clip_rect = this->clip;
  clip_rect.translate(-this->dx, -this->dy);
  return clip_rect;
}

std::optional<Color> const& TerminalGraphics::get_foreground_color() const {
  return this->foreground_color;
}

std::optional<Color> const& TerminalGraphics::get_background_color() const {
  return this->background_color;
}

void TerminalGraphics::set_foreground_color(std::optional<Color> const &color) {
  this->foreground_color = color;
}

void TerminalGraphics::set_background_color(std::optional<Color> const &color) {
  this->background_color = color;
}

Font TerminalGraphics::get_font() const {
  return this->font;
}

void TerminalGraphics::reset(const Rectangle &clip) {
  this->foreground_color = { };
  this->background_color = { };
  this->font = { };
  this->attributes = Attributes::NONE;
  this->clip = clip;
  this->dx = this->dy = 0;
}

void TerminalGraphics::set_clip_rect(const Rectangle &rect) {
  this->clip = rect;
  this->clip.translate(this->dx, this->dy);
}

bool TerminalGraphics::hit_clip_rect(int x, int y, int width, int height) const {
  auto clip_rect = get_clip_rect();
  return clip_rect.intersects(x, y, width, height);
}

void TerminalGraphics::set_font(const Font &font) {
  this->font = font;
  update_attributes();
}

Stroke TerminalGraphics::get_stroke() const {
  return this->stroke;
}

void TerminalGraphics::set_stroke(Stroke stroke) {
  this->stroke = stroke;
}

void TerminalGraphics::translate(int dx, int dy) {
  this->dx += dx;
  this->dy += dy;
}

void TerminalGraphics::update_attributes() {
  auto attributes = Attributes::NONE;
  if (this->font.get_style() & Font::BOLD) {
    attributes = Attribute::BOLD;
  }

  if (this->font.get_style() & Font::ITALIC) {
    attributes |= Attribute::ITALIC;
  }

  this->attributes = attributes;
}

void TerminalGraphics::flush() {
  this->screen.flush();
}

}
