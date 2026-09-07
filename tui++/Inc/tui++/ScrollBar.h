#pragma once

// ScrollBar - Swing's JScrollBar adapted to a terminal cell backend.
//
// A one-cell-thick track between two arrow buttons. The thumb (drawn with the
// INVERSE attribute) is proportional to the model's extent; pressing an arrow
// scrolls by the unit increment, clicking the track pages by the block
// increment, and the thumb can be dragged. All geometry is in terminal cells.

#include <tui++/BoundedRangeModel.h>
#include <tui++/Component.h>
#include <tui++/Orientation.h>

namespace tui {

// Observes screen-level drag events while a scroll bar thumb is dragged (see
// ScrollBar.cpp); declared here so ScrollBar can grant it access.
class ScrollBarDragObserver;

class ScrollBar: public Component {
  using base = Component;

public:
  enum class DragMode {
    NONE,
    ARROW_DEC, // the "up/left" arrow
    ARROW_INC, // the "down/right" arrow
    THUMB,
    TRACK_DEC,
    TRACK_INC
  };

public:
  // Orientation and how far one unit/block step scrolls. The model is the
  // same BoundedRangeModel Swing uses: value in [minimum, maximum - extent].
  ScrollBar(Orientation orientation);

  friend class ScrollBarDragObserver;

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

  std::shared_ptr<BoundedRangeModel> get_model() const {
    return this->model;
  }

  Orientation get_orientation() const {
    return this->orientation;
  }

  int get_unit_increment() const {
    return this->unit_increment;
  }

  void set_unit_increment(int increment) {
    this->unit_increment = std::max(1, increment);
  }

  int get_block_increment() const {
    return this->block_increment;
  }

  void set_block_increment(int increment) {
    this->block_increment = std::max(1, increment);
  }

  // Geometry of the thumb in local coordinates (cells), or an empty rect when
  // the bar is too small / the content fits.
  Rectangle get_thumb_rect() const;

  void set_value(int value) {
    this->model->set_value(value);
  }

  int get_value() const {
    return this->model->get_value();
  }

protected:
  // Called by make_component once the bar is owned by a shared pointer:
  // listeners are registered here (add_listener needs shared ownership to
  // reach the containing window).
  void init() override;

private:
  void on_press(int x, int y);
  void on_drag(int x, int y);
  void on_release();
  void on_wheel(int rotation);
  // The drag mode at (x, y); when the thumb is hit, `grab_offset` receives
  // the pointer's offset inside the thumb.
  DragMode hit_test(int x, int y, int &grab_offset) const;

  // While the thumb is dragged the bar observes MOUSE_DRAG / MOUSE_RELEASED
  // events on the screen: the dispatcher retargets drags to the component
  // that received the press, but components dispatch drags only to screen
  // listeners (see WindowMouseEventDispatcher), not to local listeners.
  void register_drag_observer();
  void unregister_drag_observer();

  std::shared_ptr<DefaultBoundedRangeModel> model = std::make_shared<DefaultBoundedRangeModel>();
  Orientation orientation;
  int unit_increment = 1;
  int block_increment = 10;

  DragMode drag_mode = DragMode::NONE;
  int grab_offset = 0; // drag anchor: pointer offset inside the thumb
  std::shared_ptr<EventListener<Event>> drag_observer;

protected:
  virtual void paint(Graphics &g) override;
};

}
