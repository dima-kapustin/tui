#include <tui++/Frame.h>
#include <tui++/Dialog.h>
#include <tui++/Window.h>
#include <tui++/Graphics.h>
#include <tui++/BorderLayout.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/MenuKeyboardManager.h>

#include <tui++/lookandfeel/LookAndFeel.h>

#include <tui++/event/KeyEvent.h>
#include <tui++/event/MouseEvent.h>

#include <algorithm>

namespace tui {

std::string Window::to_string() const {
  // A window's title reads better than its id; an explicit name (see
  // Component::set_name) still wins over the title.
  if (this->name.empty()) {
    if (auto const &title = get_title(); not title.empty()) {
      return describe(title);
    }
  }
  return Component::to_string();
}

void Window::add_notify() {
  if (auto parent = std::dynamic_pointer_cast<Window>(get_parent()); parent and not parent->mouse_event_dispatcher) {
    parent->add_notify();
  }

  // add_notify runs once the window is owned by a shared_ptr (Window::show /
  // pack), so the dispatcher can hold the window weakly and tell when it dies.
  this->mouse_event_dispatcher = std::make_shared<WindowMouseEventDispatcher>(std::static_pointer_cast<Window>(shared_from_this()));
  base::add_notify();
}

void Window::dispatch_event(Event &e) {
  if (this->mouse_event_dispatcher and (MOUSE_EVENT_MASK & e.id)) {
    // A mouse press ends the keyboard menu session (F10 mode, keyboard
    // navigation of an open popup) before the dispatcher runs: from the
    // press on the pointer owns the menus again (see
    // MenuKeyboardManager::handle_mouse_pressed).
    if (auto press = dynamic_cast<MousePressEvent*>(&e); press and press->id == MousePressEvent::MOUSE_PRESSED) {
      MenuKeyboardManager::single->handle_mouse_pressed(std::static_pointer_cast<Window>(shared_from_this()), *press);
    }
    this->mouse_event_dispatcher->dispatch_event(e);
    return;
  }

  // The menu system gets the first crack at every key that reaches this
  // window (key events are posted to the focused window only): an open
  // popup or an armed menu bar captures its navigation keys before the
  // window's listeners and the focused component could use them.
  if (auto key_event = dynamic_cast<KeyEvent*>(&e)) {
    if (MenuKeyboardManager::single->handle_key_event(std::static_pointer_cast<Window>(shared_from_this()), *key_event)) {
      return;
    }
  }
  base::dispatch_event(e);
}

bool Window::is_opaque() const {
  return this->background_color.has_value();
}

void Window::paint_children(Graphics &g) {
  int x = get_x(), y = get_y();
  g.translate(x, y);
  auto clip_rect = g.get_clip_rect();
  g.clip_rect(0, 0, get_width(), get_height());
  base::paint_children(g);
  g.set_clip_rect(clip_rect);
  g.translate(-x, -y);
}

std::shared_ptr<Component> Window::get_focus_owner() const {
  return is_focused() ? KeyboardFocusManager::single->get_focus_owner() : nullptr;
}

std::shared_ptr<Component> Window::get_most_recent_focus_owner() const {
  if (is_focused()) {
    return get_focus_owner();
  } else if (auto most_recent = KeyboardFocusManager::single->get_most_recent_focus_owner(std::dynamic_pointer_cast<const Window>(shared_from_this()))) {
    return most_recent;
  } else {
    return is_focusable_window() ? get_focus_traversal_policy()->get_initial_component(std::dynamic_pointer_cast<Window>(const_cast<Window*>(this)->shared_from_this())) : nullptr;
  }
}

bool Window::is_focusable_window() const {
  // If a Window/Frame/Dialog was made non-focusable, then it is always
  // non-focusable.
  if (not this->focusable_window_state) {
    return false;
  }

  // All other tests apply only to Windows.
  if (is_a<Frame>(this) or is_a<Dialog>(this)) {
    return true;
  }

  // A Window must have at least one Component in its root focus
  // traversal cycle to be focusable.
  if (not get_focus_traversal_policy()->get_default_component(const_cast<Window*>(this)->shared_from_this())) {
    return false;
  }

  // A Window's nearest owning Frame or Dialog must be showing on the
  // screen.
  for (auto owner = get_owner(); owner; owner = owner->get_owner()) {
    // TODO
//    if (owner instanceof Frame || owner instanceof Dialog) {
    return owner->is_showing();
//    }
  }

  return false;
}

void Window::set_focusable_window_state(bool state) {
  bool old_focusable_window_state = this->focusable_window_state;

  this->focusable_window_state = state;

  if (old_focusable_window_state and not this->focusable_window_state and is_focused()) {
    for (auto owner = get_owner(); owner; owner = owner->get_owner()) {
      auto to_focus = KeyboardFocusManager::single->get_most_recent_focus_owner(owner);
      if (to_focus and to_focus->request_focus(false, FocusEvent::Cause::ACTIVATION)) {
        return;
      }
    }

    KeyboardFocusManager::single->clear_focus_owner();
  }
}

void Window::set_always_on_top(bool value) {
  this->always_on_top = value;
}

void Window::to_front() {
  screen.to_front(std::dynamic_pointer_cast<Window>(shared_from_this()));
}

void Window::show() {
  if (not this->mouse_event_dispatcher) {
    add_notify();
  }
  validate_unconditionally();
  this->in_show = true;
  if (this->visible) {
    to_front();
  } else {
    this->before_first_show = false;
    // TODO
    // close_splash_screen();
    // Dialog::check_should_be_blocked(shared_from_this());
    base::show();
    screen.show_window(std::static_pointer_cast<Window>(shared_from_this()));
  }
  this->in_show = false;

  if (not this->opened) {
    fire_event<WindowEvent>(std::static_pointer_cast<Window>(shared_from_this()), WindowEvent::WINDOW_OPENED);
    this->opened = true;
  }
}

void Window::hide() {
  screen.hide_window(std::dynamic_pointer_cast<Window>(shared_from_this()));

  // The screen keeps a registered mouse dispatcher alive; once the window is
  // hidden it must not keep observing (or be kept alive by) the screen, and a
  // dispatcher must never outlive the window it points at.
  if (this->mouse_event_dispatcher) {
    this->mouse_event_dispatcher->unregister();
  }

  base::hide();
}

void Window::init() {
  Component::init();
  this->visible = false;
  set_layout(std::make_shared<BorderLayout>());
  set_root_pane(create_root_pane());
  if (this->owner) {
    this->owner->add_owned_window(std::static_pointer_cast<Window>(shared_from_this()));
    set_always_on_top(this->owner->is_always_on_top());
  }

  set_focus_traversal_policy(KeyboardFocusManager::single->get_default_focus_traversal_policy());

  // The shadow this window's kind casts: a popup menu, a combo box dropdown
  // and a dialog shade the window below them, a frame (which fills the
  // screen) does not. Installed rather than read per paint, the way every
  // other theme default reaches a component; set_shadow replaces it for one
  // window, and the global switch is applied when the shadow is painted.
  laf::LookAndFeel::install(this, "Shadow", laf::LookAndFeel::get<std::optional<Shadow>>(get_shadow_key()));
}

std::optional<Shadow> Window::get_shadow() const {
  if (not Shadow::is_enabled() or not this->shadow.has_value()) {
    return std::nullopt;
  }
  return this->shadow.value();
}

void Window::set_shadow(std::optional<Shadow> const &shadow) {
  auto old_area = get_shadow_area();
  this->shadow = shadow;

  // A shadow lies outside the window, where the window's own repaint() never
  // reaches: damage the area the shadow had and the area it takes, so the
  // cells it leaves behind are repainted by the window below. (With the
  // global switch off there is nothing to paint either way, and switching it
  // back on repaints the whole screen.)
  auto area = old_area | get_shadow_area();
  if (is_showing() and not area.empty()) {
    screen.add_damage(area, shared_from_this());
  }
}

Rectangle Window::get_shadow_area() const {
  if (auto shadow = get_shadow()) {
    return shadow->get_area(get_bounds());
  }
  return { };
}

void Window::paint_shadow(Graphics &g) {
  auto shadow = get_shadow();
  if (not shadow) {
    return;
  }

  auto area = shadow->get_area(get_bounds()) & g.get_clip_rect();
  if (area.empty()) {
    return;
  }

  auto paint = [&](Rectangle const &part) {
    if (not part.empty()) {
      g.blend_rect(part, shadow->color, shadow->opacity);
    }
  };

  // Only the parts of the shadow the window does not cover are visible -- and
  // the window may leave cells of its own face unpainted (a translucent
  // window, a panel with no colors), which must not be tinted by the shadow
  // running underneath it. So paint the area as the bands above, right, below
  // and left of the window's rectangle (in whatever order: they do not
  // overlap).
  auto window = get_bounds();
  auto below_top = std::max(area.y, window.bottom());
  paint({ area.x, area.y, area.width, std::min(area.bottom(), window.y) - area.y });

  auto middle_top = std::max(area.y, window.y);
  auto middle_bottom = std::min(area.bottom(), window.bottom());
  if (middle_bottom > middle_top) {
    paint({ area.x, middle_top, window.x - area.x, middle_bottom - middle_top });
    paint({ window.right(), middle_top, area.right() - window.right(), middle_bottom - middle_top });
  }

  paint({ area.x, below_top, area.width, area.bottom() - below_top });
}

void Window::pack() {
  if (not this->mouse_event_dispatcher) {
    add_notify();
  }

  // The window's own layout is redirected to the root pane, so its preferred
  // size is empty; size the window from the root pane (the content plus the
  // window insets and menu bar), which is what a popup needs to fit its menu.
  auto size = this->root_pane->get_preferred_size();
  if (size.width <= 0 or size.height <= 0) {
    size = get_preferred_size();
  }
  set_size(size);
  validate_unconditionally();
}

void Window::add_owned_window(const std::shared_ptr<Window> &w) {
  with_tree_locked([&w, this] {
    this->owned_windows.emplace_back(w);
  });
}

void Window::remove_owned_window(const std::shared_ptr<Window> &w) {
  with_tree_locked([&w, this] {
    std::erase_if(this->owned_windows, [&w](auto const &candidate) {
      return candidate.expired() or candidate.lock() == w;
    });
  });
}

void Window::add_impl(const std::shared_ptr<Component> &c, const Constraints &constraints, int z_order) noexcept (false) {
  if (c == this->root_pane) {
    base::add_impl(c, constraints, z_order);
  } else {
    this->root_pane->get_content_pane()->add(c, constraints, z_order);
  }
}

void Window::remove(const std::shared_ptr<Component> &c) {
  if (c == this->root_pane) {
    base::remove(c);
  } else {
    this->root_pane->get_content_pane()->remove(c);
  }
}

void Window::set_layout(const std::shared_ptr<Layout> &layout) {
  if (this->root_pane) {
    this->root_pane->get_content_pane()->set_layout(layout);
  } else {
    base::set_layout(layout);
  }
}

std::shared_ptr<RootPane> Window::create_root_pane() const {
  auto root_pane = make_component<RootPane>();
  root_pane->set_opaque(true);
  return root_pane;
}

std::shared_ptr<RootPane> Window::get_root_pane() const {
  return this->root_pane;
}

std::shared_ptr<Component> Window::get_content_pane() const {
  return this->root_pane->get_content_pane();
}

void Window::set_content_pane(const std::shared_ptr<Component> &content_pane) {
  this->root_pane->set_content_pane(content_pane);
}

std::shared_ptr<LayeredPane> Window::get_layered_pane() const {
  return this->root_pane->get_layered_pane();
}

void Window::set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) {
  this->root_pane->set_layered_pane(layered_pane);
}

std::shared_ptr<Component> Window::get_glass_pane() const {
  return this->root_pane->get_glass_pane();
}

void Window::set_glass_pane(const std::shared_ptr<Component> &glass_pane) {
  this->root_pane->set_glass_pane(glass_pane);
}

void Window::set_root_pane(const std::shared_ptr<RootPane> &root_pane) {
  if (this->root_pane) {
    remove(this->root_pane);
  }

  this->root_pane = root_pane;
  if (this->root_pane) {
    add(this->root_pane, BorderLayout::CENTER);
  }
}

}
