#include <tui++/ScrollBar.h>
#include <tui++/Graphics.h>
#include <tui++/Attributes.h>
#include <tui++/Screen.h>
#include <tui++/Component.h>
#include <tui++/Symbols.h>
#include <tui++/event/MouseEvent.h>

namespace tui {

// Drags (and the release that ends them) are retargeted to the component that
// received the press, but a Component dispatches them only to screen
// listeners; see WindowMouseEventDispatcher. This observer is registered on
// the screen while the thumb is dragged and translates nothing: the
// dispatcher already delivers drag coordinates in the press target's (the
// bar's) local space. Defined outside the anonymous namespace so the friend
// declaration in the header (tui::ScrollBarDragObserver) names the same
// class.
class ScrollBarDragObserver final: public EventListener<Event>, public std::enable_shared_from_this<ScrollBarDragObserver> {
  std::weak_ptr<ScrollBar> bar;

public:
  explicit ScrollBarDragObserver(std::weak_ptr<ScrollBar> const &bar) :
      bar(bar) {
  }

  void event_dispatched(Event &e) override {
    if (auto bar = this->bar.lock()) {
      auto from_bar = std::dynamic_pointer_cast<Component>(e.source) == bar;
      if (e.id == MouseDragEvent::MOUSE_DRAGGED) {
        if (from_bar) {
          auto &mouse = static_cast<MouseDragEvent&>(e);
          bar->on_drag(mouse.x, mouse.y);
        }
      } else if (e.id == MousePressEvent::MOUSE_RELEASED and from_bar) {
        bar->on_release();
        bar->unregister_drag_observer();
      }
      return;
    }
    // The bar died while being dragged (window closed): stop observing.
    screen.remove_listener(shared_from_this());
  }
};

ScrollBar::ScrollBar(Orientation orientation) :
    orientation(orientation) {
  set_background_color(Color { 42, 44, 52 });
  set_foreground_color(Color { 150, 150, 158 });

  // The model listener only touches the model's own listener list, so it is
  // safe in the constructor; the component listeners below need shared
  // ownership (add_listener reaches the containing window) and are therefore
  // registered in init().
  this->model->add_change_listener([this] {
    repaint();
  });
}

void ScrollBar::init() {
  base::init();
  // The bar reacts to presses on itself; the release ends any drag.
  add_listener([this](MousePressEvent &e) {
    if (e.id == MousePressEvent::MOUSE_PRESSED) {
      on_press(e.x, e.y);
      e.consume();
    } else {
      on_release();
    }
  });
  add_listener([this](MouseWheelEvent &e) {
    on_wheel(e.wheel_rotation);
    e.consume();
  });
  // The thumb drag is observed on the screen (see ScrollBarDragObserver), but
  // the window dispatcher retargets a drag to the press target only when the
  // window enables MOUSE_DRAG events. Enabling it here (there is no local
  // listener, so the drag is dispatched to the screen observer only) makes
  // the window forward drags to this bar.
  enable_events(EventType::MOUSE_DRAG);
}

void ScrollBar::register_drag_observer() {
  if (not this->drag_observer) {
    this->drag_observer = std::make_shared<ScrollBarDragObserver>(std::static_pointer_cast<ScrollBar>(shared_from_this()));
    screen.add_listener(EventType::MOUSE_DRAG | EventType::MOUSE_PRESS, this->drag_observer);
  }
}

void ScrollBar::unregister_drag_observer() {
  if (this->drag_observer) {
    screen.remove_listener(this->drag_observer);
    this->drag_observer.reset();
  }
}

// Geometry: one arrow cell at each end, the track between them. The thumb is
// proportional to extent / (maximum - minimum).
static bool is_vertical(Orientation orientation) {
  return orientation == Orientation::VERTICAL;
}

namespace {

struct BarGeometry {
  int span;       // track length in cells
  int thumb_len;  // thumb length in cells, >= 1
  int thumb_origin; // thumb start inside the track
  int range;      // maximum - minimum - extent
};

BarGeometry bar_geometry(ScrollBar const &bar, Rectangle const &bounds) {
  auto model = bar.get_model();
  auto orientation = bar.get_orientation();
  auto length = is_vertical(orientation) ? bounds.height : bounds.width;
  auto range = model->get_maximum() - model->get_minimum() - model->get_extent();
  auto extent = model->get_extent();

  BarGeometry g { };
  g.range = range;
  g.span = std::max(0, length - 2);
  if (range <= 0 or g.span <= 0) {
    g.span = std::max(0, length - 2);
    g.thumb_len = g.span;
    g.thumb_origin = 0;
    return g;
  }
  auto value = model->get_value() - model->get_minimum();
  auto scaled = (std::uint64_t(value) * g.span) / range;
  g.thumb_len = std::max(1, int((std::uint64_t(extent) * (g.span + 1)) / (range + extent)));
  g.thumb_len = std::min(g.thumb_len, g.span);
  g.thumb_origin = std::min(int(scaled), g.span - g.thumb_len);
  return g;
}

}

Rectangle ScrollBar::get_thumb_rect() const {
  auto g = bar_geometry(*this, get_bounds());
  if (is_vertical(this->orientation)) {
    return { 0, 1 + g.thumb_origin, get_width(), g.thumb_len };
  }
  return { 1 + g.thumb_origin, 0, g.thumb_len, get_height() };
}

void ScrollBar::paint(Graphics &g) {
  auto w = get_width();
  auto h = get_height();

  if (auto bg = get_background_color()) {
    g.set_background_color(bg);
    g.fill_rect(0, 0, w, h);
  }

  auto fg = get_foreground_color();
  g.set_foreground_color(fg);

  // Arrow cells. A terminal has no "up"/"down" arrows in ASCII; the caret
  // (^) and the letter (v) it fell back to before have very different visual
  // heights, so use the dedicated arrow glyphs (single-cell) for all four.
  if (is_vertical(this->orientation)) {
    if (h >= 1) {
      g.draw_char(Symbols::ARROW_UP, 0, 0);
    }
    if (h >= 2) {
      g.draw_char(Symbols::ARROW_DOWN, 0, h - 1);
    }
    auto thumb = get_thumb_rect();
    for (auto y = thumb.y; y < thumb.bottom() and y < h - 1; ++y) {
      for (auto x = 0; x < w; ++x) {
        g.draw_char(Char(' '), x, y, Attribute::INVERSE);
      }
    }
  } else {
    if (w >= 1) {
      g.draw_char(Symbols::ARROW_LEFT, 0, 0);
    }
    if (w >= 2) {
      g.draw_char(Symbols::ARROW_RIGHT, w - 1, 0);
    }
    auto thumb = get_thumb_rect();
    for (auto y = 0; y < h; ++y) {
      for (auto x = thumb.x; x < thumb.right() and x < w - 1; ++x) {
        g.draw_char(Char(' '), x, y, Attribute::INVERSE);
      }
    }
  }
}

ScrollBar::DragMode ScrollBar::hit_test(int x, int y, int &grab_offset) const {
  auto g = bar_geometry(*this, get_bounds());
  auto vertical = is_vertical(this->orientation);
  auto pos = vertical ? y : x;

  if (vertical) {
    if (y < 1) {
      return DragMode::ARROW_DEC;
    }
    if (y >= get_height() - 1 and get_height() > 1) {
      return DragMode::ARROW_INC;
    }
  } else {
    if (x < 1) {
      return DragMode::ARROW_DEC;
    }
    if (x >= get_width() - 1 and get_width() > 1) {
      return DragMode::ARROW_INC;
    }
  }

  if (g.span > 0 and pos >= 1 + g.thumb_origin and pos < 1 + g.thumb_origin + g.thumb_len) {
    grab_offset = pos - (1 + g.thumb_origin);
    return DragMode::THUMB;
  }
  if (pos < 1 + g.thumb_origin) {
    return DragMode::TRACK_DEC;
  }
  return DragMode::TRACK_INC;
}

void ScrollBar::on_press(int x, int y) {
  this->drag_mode = hit_test(x, y, this->grab_offset);
  if (this->drag_mode == DragMode::THUMB) {
    // While the thumb is held, follow the pointer through screen-level drag
    // events (they are retargeted to this bar by the window dispatcher).
    register_drag_observer();
  }
  switch (this->drag_mode) {
  case DragMode::ARROW_DEC:
    this->model->set_value(this->model->get_value() - this->unit_increment);
    break;
  case DragMode::ARROW_INC:
    this->model->set_value(this->model->get_value() + this->unit_increment);
    break;
  case DragMode::TRACK_DEC:
    this->model->set_value(this->model->get_value() - this->block_increment);
    break;
  case DragMode::TRACK_INC:
    this->model->set_value(this->model->get_value() + this->block_increment);
    break;
  default:
    break;
  }
}

void ScrollBar::on_drag(int x, int y) {
  if (this->drag_mode != DragMode::THUMB) {
    return;
  }
  auto g = bar_geometry(*this, get_bounds());
  if (g.range <= 0 or g.span <= 0) {
    return;
  }
  auto pos = is_vertical(this->orientation) ? y : x;
  auto track_origin = 1;
  auto track_len = g.span - g.thumb_len;
  if (track_len <= 0) {
    return;
  }
  auto value = pos - track_origin - this->grab_offset;
  value = std::clamp(value, 0, track_len);
  auto new_value = this->model->get_minimum() + int((std::uint64_t(value) * g.range + track_len / 2) / track_len);
  this->model->set_value(new_value);
}

void ScrollBar::on_release() {
  this->drag_mode = DragMode::NONE;
  unregister_drag_observer();
}

void ScrollBar::on_wheel(int rotation) {
  if (rotation != 0) {
    this->model->set_value(this->model->get_value() + this->block_increment * rotation);
  }
}

}
