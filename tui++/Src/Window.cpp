#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/KeyboardFocusManager.h>

namespace tui {

void Window::paint_components(Graphics &g) {
  int x = get_x(), y = get_y();
  g.translate(x, y);
  auto clip_rect = g.get_clip_rect();
  g.clip_rect(0, 0, get_width(), get_height());
  base::paint_components(g);
  g.set_clip_rect(clip_rect);
  g.translate(-x, -y);
}

std::shared_ptr<Component> Window::get_focus_owner() const {
  return is_focused() ? KeyboardFocusManager::get_focus_owner() : nullptr;
}
}
