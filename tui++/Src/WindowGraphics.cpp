#include <tui++/WindowGraphics.h>

namespace tui {

void WindowGraphics::clip_rect(int x, int y, int width, int height) {
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

std::unique_ptr<Graphics> WindowGraphics::create() {
  return std::unique_ptr<Graphics> { new WindowGraphics(this->clip, this->dx, this->dx) };
}

std::unique_ptr<Graphics> WindowGraphics::create(int x, int y, int width, int height) {
  auto g = create();
  g->translate(x, y);
  g->clip_rect(0, 0, width, height);
  return g;
}

void WindowGraphics::draw_char(int ch, int x, int y, int attributes) {
  if (this->clip.contains(x + this->dx, y + this->dy)) {
//    Toolkit.getDefaultToolkit().setCursor(x + this->dx, y + this->dy);
//    Toolkit.getDefaultToolkit().drawChar(ch, attributes | this->attributes, getCursesColors());
  }
}

void WindowGraphics::draw_hline(int x, int y, int length, const std::string &chr) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, length, 1)) {
//    Toolkit.getDefaultToolkit().setCursor(rect.x, rect.y);
//    for (int i = 0; i < rect.width; ++i) {
//      Toolkit.getDefaultToolkit().drawChar(chr, this->attributes, getCursesColors());
//    }
  }
}

void WindowGraphics::draw_rect(int x, int y, int width, int height) {
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
  if (top >= clip_top && top < clip_bottom) {
//    Toolkit.getDefaultToolkit().setCursor(max_left, top);
    if (left == max_left) {
      // upper left corner
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_ULCORNER, this->attributes, getCursesColors());
    }
    for (int i = max_left + 1; i < min_right - 1; i++) {
      // top horizontal line
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_HLINE, this->attributes, getCursesColors());
    }
    if (right == min_right) {
      // upper left corner
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_URCORNER, this->attributes, getCursesColors());
    }
  }

  // If the bottom of the box is outside the clipping rectangle, don't bother
  if (bottom >= clip_top && bottom <= clip_bottom) {
//    Toolkit.getDefaultToolkit().setCursor(max_left, bottom - 1);
    if (left == max_left) {
      // lower left corner
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_LLCORNER, this->attributes, getCursesColors());
    }
    for (int i = max_left + 1; i < min_right - 1; i++) {
      // bottom horizontal line
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_HLINE, this->attributes, getCursesColors());
    }
    if (right == min_right) {
      // lower left corner
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_LRCORNER, this->attributes, getCursesColors());
    }
  }

  // If the left side of the box is outside the clipping rectangle, don't bother.
  if (left >= clip_left && left < clip_right) {
    for (int i = max_top + 1; i < min_bottom - 1; i++) {
//      Toolkit.getDefaultToolkit().setCursor(left, i);
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_VLINE, this->attributes, getCursesColors());
    }
  }
  //
  // If the right side of the box is outside the clipping rectangle, don't bother.
  if (right >= clip_left && right <= clip_right) {
    for (int i = max_top + 1; i < min_bottom - 1; i++) {
//      Toolkit.getDefaultToolkit().setCursor(right - 1, i);
//      Toolkit.getDefaultToolkit().drawChar(Toolkit.ACS_VLINE, this->attributes, getCursesColors());
    }
  }
}

void WindowGraphics::draw_string(const std::string &str, int x, int y, int attributes) {
  x += this->dx;
  y += this->dy;
  if (auto rect = this->clip.get_intersection(x, y, str.length(), 1)) {
//    Toolkit.getDefaultToolkit().setCursor(rect.x, rect.y);
//    Toolkit.getDefaultToolkit().drawString(str.substring(rect.x - x, rect.x - x + rect.width), attributes | this->attributes, getCursesColors());
  }
}

void WindowGraphics::draw_vline(int x, int y, int length, const std::string &chr) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, 1, length)) {
//    for (int i = 0; i < rect.height; ++i) {
//      Toolkit.getDefaultToolkit().setCursor(rect.x, rect.y + i);
//      Toolkit.getDefaultToolkit().drawChar(chr, this->attributes, getCursesColors());
//    }
  }
}

void WindowGraphics::fill_rect(int x, int y, int width, int height) {
  if (auto rect = this->clip.get_intersection(x + this->dx, y + this->dy, width, height)) {
//    for (int j = rect.y, bottom = rect.y + rect.height; j < bottom; j++) {
//      Toolkit.getDefaultToolkit().setCursor(rect.x, j);
//      for (int i = rect.x, right = rect.x + rect.width; i < right; i++) {
//        Toolkit.getDefaultToolkit().drawChar(' ', this->attributes, getCursesColors());
//      }
//    }
  }
}

Rectangle WindowGraphics::get_clip_rect() const {
  auto clip_rect = this->clip;
  clip_rect.translate(-this->dx, -this->dy);
  return clip_rect;
}

Color WindowGraphics::get_foreground_color() const {
  return this->foreground_color;
}

Color WindowGraphics::get_background_color() const {
  return this->background_color;
}

Font WindowGraphics::get_font() const {
  return this->font;
}

void WindowGraphics::reset(const Rectangle &clip) {
  this->foreground_color = { };
  this->background_color = { };
  this->font = { };
  this->attributes = 0;
  this->clip = clip;
  this->dx = this->dy = 0;
}

void WindowGraphics::set_clip_rect(const Rectangle &rect) {
  this->clip = rect;
  this->clip.translate(this->dx, this->dy);
}

void WindowGraphics::set_foreground_color(const Color &color) {
  this->foreground_color = color;
}

void WindowGraphics::set_background_color(const Color &color) {
  this->background_color = color;
}

void WindowGraphics::set_font(const Font &font) {
  this->font = font;
  update_attributes();
}

void WindowGraphics::translate(int dx, int dy) {
  this->dx += dx;
  this->dy += dy;
}

void WindowGraphics::update_attributes() {
  this->attributes = 0;
//  if (this->font != null) {
//    int style = this->font.getStyle();
//    if ((style & Font.BOLD) != 0) {
//      this->attributes |= Toolkit.A_BOLD;
//    }
//  }
//  if (this->color != null) {
//    if (this->color.foreground.isBright() || this->color.background.isBright()) {
//      this->attributes |= Toolkit.A_BOLD;
//    }
//  }
}

}
