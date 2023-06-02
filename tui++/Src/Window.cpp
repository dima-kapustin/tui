#include <tui++/Window.h>
#include <tui++/Graphics.h>

namespace tui {

void Window::paint_children(Graphics &g) {
  int x = get_x(), y = get_y();
  g.translate(x, y);
  auto clip_rect = g.get_clip_rect();
  g.clip_rect(0, 0, get_width(), get_height());
  base::paint_children(g);
  g.set_clip_rect(clip_rect);
  g.translate(-x, -y);
}

}
