#pragma once

#include <tui++/ComponentOrientation.h>
#include <memory>
#include <vector>

#include <tui++/Point.h>
#include <tui++/Dimension.h>

#include <tui++/Object.h>

namespace tui {

class Border;
class Graphics;

class Component: public Object {
protected:
  std::string name;

  std::shared_ptr<Border> border;

  std::vector<std::shared_ptr<Component>> components;
  std::vector<std::weak_ptr<Component>> container;

  struct {
    bool was_focus_owner :1 = false;
    bool is_valid :1 = false;
    bool is_minimum_size_set :1 = false;
    bool is_preferred_size_set :1 = false;
  };

  float alignment_x = LEFT_ALIGNMENT;
  float alignment_y = TOP_ALIGNMENT;

  ComponentOrientation orientation = ComponentOrientation::UNKNOWN;

  Point location { };
  Dimension size { };

  Dimension minimum_size { };
  Dimension preferred_size { };

  Property<bool> enabled { this, "enabled" };
  Property<bool> visible { this, "visible" };

public:
  constexpr static float TOP_ALIGNMENT = 0;
  constexpr static float BOTTOM_ALIGNMENT = 1.0;
  constexpr static float CENTER_ALIGNMENT = 0.5;
  constexpr static float LEFT_ALIGNMENT = 0;
  constexpr static float RIGHT_ALIGNMENT = 1.0;

  enum InvocationPolicy {
    // Command should be invoked when the component has the focus.
    WHEN_FOCUSED = 0,
    // Command should be invoked when the receiving component is an ancestor of the focused component or is itself the focused component.
    WHEN_ANCESTOR_OF_FOCUSED_WIDGET = 1,
    // Command should be invoked when the receiving component is in the window that has the focus or is itself the focused component.
    WHEN_IN_FOCUSED_WINDOW = 2
  };

protected:
  virtual void show() {
  }

  virtual void hide() {
  }

public:
  virtual ~Component() {

  }

  virtual void paint(Graphics &g) = 0;

  void set_visible(bool visible) {
    if (visible) {
      if (not this->visible) {
        show();
        this->visible = true;
      }
    } else {
      if (this->visible) {
        hide();
        this->visible = false;
      }
    }
  }
};

}
