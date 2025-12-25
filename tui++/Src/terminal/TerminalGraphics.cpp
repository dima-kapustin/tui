#include <tui++/CharIterator.h>
#include <tui++/terminal/TerminalScreen.h>
#include <tui++/terminal/TerminalGraphics.h>

namespace tui {

TerminalGraphics::BoxCharacters TerminalGraphics::LIGHT_BOX = //
    { .top_left = BoxDrawing::DOWN_AND_RIGHT_LIGHT, //
      .top = BoxDrawing::HORIZONTAL_LIGHT, //
      .top_right = BoxDrawing::DOWN_AND_LEFT_LIGHT, //
      .right = BoxDrawing::VERTICAL_LIGHT, //
      .bottom_right = BoxDrawing::UP_AND_LEFT_LIGHT, //
      .bottom = BoxDrawing::HORIZONTAL_LIGHT, //
      .bottom_left = BoxDrawing::UP_AND_RIGHT_LIGHT, //
      .left = BoxDrawing::VERTICAL_LIGHT, //
      .horizontal = BoxDrawing::HORIZONTAL_LIGHT, //
      .vertical = BoxDrawing::VERTICAL_LIGHT };

TerminalGraphics::BoxCharacters TerminalGraphics::HEAVY_BOX = //
    { .top_left = BoxDrawing::DOWN_AND_RIGHT_HEAVY, //
      .top = BoxDrawing::HORIZONTAL_HEAVY, //
      .top_right = BoxDrawing::DOWN_AND_LEFT_HEAVY, //
      .right = BoxDrawing::VERTICAL_HEAVY, //
      .bottom_right = BoxDrawing::UP_AND_LEFT_HEAVY, //
      .bottom = BoxDrawing::HORIZONTAL_HEAVY, //
      .bottom_left = BoxDrawing::UP_AND_RIGHT_HEAVY, //
      .left = BoxDrawing::VERTICAL_HEAVY, //
      .horizontal = BoxDrawing::HORIZONTAL_HEAVY, //
      .vertical = BoxDrawing::VERTICAL_HEAVY };

TerminalGraphics::BoxCharacters TerminalGraphics::DOUBLE_BOX = //
    { .top_left = BoxDrawing::DOWN_AND_RIGHT_DOUBLE, //
      .top = BoxDrawing::HORIZONTAL_DOUBLE, //
      .top_right = BoxDrawing::DOWN_AND_LEFT_DOUBLE, //
      .right = BoxDrawing::VERTICAL_DOUBLE, //
      .bottom_right = BoxDrawing::UP_AND_LEFT_DOUBLE, //
      .bottom = BoxDrawing::HORIZONTAL_DOUBLE, //
      .bottom_left = BoxDrawing::UP_AND_RIGHT_DOUBLE, //
      .left = BoxDrawing::VERTICAL_DOUBLE, //
      .horizontal = BoxDrawing::HORIZONTAL_DOUBLE, //
      .vertical = BoxDrawing::VERTICAL_DOUBLE };

TerminalGraphics::BoxCharacters TerminalGraphics::ROUNDED_LIGHT_BOX = //
    { .top_left = BoxDrawing::DOWN_AND_RIGHT_ARC, //
      .top = BoxDrawing::HORIZONTAL_LIGHT, //
      .top_right = BoxDrawing::DOWN_AND_LEFT_ARC, //
      .right = BoxDrawing::VERTICAL_LIGHT, //
      .bottom_right = BoxDrawing::UP_AND_LEFT_ARC, //
      .bottom = BoxDrawing::HORIZONTAL_LIGHT, //
      .bottom_left = BoxDrawing::UP_AND_RIGHT_ARC, //
      .left = BoxDrawing::VERTICAL_LIGHT, //
      .horizontal = BoxDrawing::HORIZONTAL_LIGHT, //
      .vertical = BoxDrawing::VERTICAL_LIGHT };

TerminalGraphics::TerminalGraphics(TerminalScreen &screen) :
    TerminalGraphics(screen, Rectangle { 0, 0, screen.get_width(), screen.get_height() }, 0, 0) {
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

  this->clip.set_bounds(clip_left, clip_top, clip_width, clip_height);
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

void TerminalGraphics::draw_char(const Char &c, int x, int y, const Attributes &attributes) {
  if (this->clip.contains(x + this->dx, y + this->dy)) {
    this->screen.draw_char(c, x + this->dx, y + this->dy, this->foreground_color, this->background_color, this->attributes | attributes);
  }
}

void TerminalGraphics::draw_hline(int x, int y, int length, const Attributes &attributes) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, length, 1)) {
    auto &chars = get_box_chars(this->stroke);
    for (int x = rect.get_left(), right = rect.get_right(); x < right; ++x) {
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

void TerminalGraphics::draw_string(const std::string &str, int x, int y, const Attributes &attributes) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, util::glyph_width(str), 1)) {
    auto right = rect.get_right();
    for (auto &&ch : to_chars(str)) {
      auto glyph_width = ch.glyph_width();
      if ((x + glyph_width) >= rect.x) {
        this->screen.draw_char(ch, x, y, this->foreground_color, this->background_color, this->attributes | attributes);
      }
      x += glyph_width;
      if (x >= right) {
        break;
      }
    }
  }
}

void TerminalGraphics::draw_vline(int x, int y, int length, const Attributes &attributes) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, 1, length)) {
    auto &chars = get_box_chars(this->stroke);
    for (auto y = rect.get_top(), bottom = rect.get_bottom(); y < bottom; ++y) {
      this->screen.draw_char(chars.vertical, rect.x, y, this->foreground_color, this->background_color, this->attributes | attributes);
    }
  }
}

void TerminalGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, width, height)) {
    for (auto j = rect.get_top(), bottom = rect.get_bottom(); j < bottom; j++) {
      for (auto i = rect.get_left(), right = rect.get_right(); i < right; i++) {
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

Color TerminalGraphics::get_foreground_color() const {
  return this->foreground_color;
}

Color TerminalGraphics::get_background_color() const {
  return this->background_color;
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

void TerminalGraphics::set_foreground_color(const Color &color) {
  this->foreground_color = color;
}

void TerminalGraphics::set_background_color(const Color &color) {
  this->background_color = color;
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
  this->attributes = Attributes::NONE;
  if (this->font.get_style() & Font::BOLD) {
    this->attributes |= Attribute::BOLD;
  }

  if (this->font.get_style() & Font::ITALIC) {
    this->attributes |= Attribute::ITALIC;
  }
}

void TerminalGraphics::flush() {
  this->screen.flush();
}

}
