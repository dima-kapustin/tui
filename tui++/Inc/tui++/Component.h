#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <exception>

#include <tui++/Point.h>
#include <tui++/Layout.h>
#include <tui++/Dimension.h>
#include <tui++/Rectangle.h>
#include <tui++/ComponentOrientation.h>

#include <tui++/Object.h>

namespace tui {

class Border;
class Window;
class Graphics;
class Component;

template<typename T>
constexpr bool is_component_v = std::is_base_of_v<Component, T>;

class Component: public Object, public std::enable_shared_from_this<Component> {
  static std::mutex tree_mutex;

protected:
  std::string name;

  std::shared_ptr<Border> border;

  std::vector<std::shared_ptr<Component>> components;
  std::weak_ptr<Component> parent;

  std::shared_ptr<Layout> layout;

  /**
   * The component inside this Component that currently has the input focus (or, if the input focus is
   * currently outside this Component, the component to which focus will return if and when this Component regains focus).
   */
  std::weak_ptr<Component> focus_component;

  struct {
    int was_focus_owner :1 = false;
    int is_foreground_set :1 = false;
    int is_background_set :1 = false;
    int is_minimum_size_set :1 = false;
    int is_preferred_size_set :1 = false;
  };

  float alignment_x = LEFT_ALIGNMENT;
  float alignment_y = TOP_ALIGNMENT;

  ComponentOrientation orientation = ComponentOrientation::UNKNOWN;

  Point location { };
  Dimension size { };

  Dimension minimum_size { };
  Dimension preferred_size { };

  /**
   * A flag that is set to true when the container is laid out, and set to false when a component is added or removed from the container
   * (indicating that it needs to be laid out again).
   */
  bool valid = false;

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

private:
  void repaint_parent_if_needed(int old_x, int old_y, int old_width, int old_height) {
    if (auto parent = this->parent.lock(); parent and is_showing()) {
      // Have the parent redraw the area this component occupied.
      parent->repaint(old_x, old_y, old_width, old_height);
      // Have the parent redraw the area this component *now* occupies.
      repaint();
    }
  }

  /**
   * Invalidates the component unless it is already invalid.
   */
  void invalidate_if_valid() {
    if (is_valid()) {
      invalidate();
    }
  }

protected:
  Component() {
  }

protected:
  static bool is_window(const std::shared_ptr<Component> &component);

  virtual void show() {
  }

  virtual void hide() {
  }

  void add_impl(const std::shared_ptr<Component> &c, const std::any &constraints) noexcept (false) {
    if (is_window(c)) {
      throw std::runtime_error("Adding a window to a container");
    }

    for (auto parent = shared_from_this(); parent; parent = parent->parent.lock()) {
      if (parent == c) {
        throw std::runtime_error("Adding component's parent to itself");
      }
    }

    // Reparent the component
    if (auto parent = c->get_parent()) {
      parent->remove(c);
    }

    // Add the specified component to the list of components
    // in this container
    this->components.emplace_back(c);

    // Set this container as the parent of the component
    c->set_parent(shared_from_this());

    invalidate();

    if (is_displayable()) {
      c->add_notify();
    }

    if (this->layout) {
      this->layout->add_layout_component(c, constraints);
    }
  }

  virtual void add_notify() {
  }

  void remove_impl(const std::shared_ptr<Component> &component) {
    if (is_displayable()) {
      component->remove_notify();
    }

    if (this->layout) {
      this->layout->remove_layout_component(component);
    }

    this->components.erase( //
        std::remove(this->components.begin(), this->components.end(), component), //
        this->components.end());

    component->set_parent(nullptr);
  }

  virtual void remove_notify() {
  }

  /**
   * Set the parent container of this component. This is intended to be called by Container objects only. Note that we use a WeakReference
   * so that the parent can be garbage- collected when there are no more strong references to it.
   */
  void set_parent(const std::shared_ptr<Component> &component) {
    this->parent = component;

    if (component) {
      // If this component's colors have not been set yet, inherit the parent's colors.
//    if (not get_foreground()) {
//      set_foreground(component->get_foreground());
//    }
//
//    if (not get_background()) {
//      set_background(component->get_background());
//    }
    }
  }

  /**
   * Determines whether this component will be displayed on the screen, if it's displayable
   *
   * @return <code>true</code> if the component and all of its ancestors are visible, <code>false</code> otherwise
   */
  bool is_recursively_visible() const {
    if (this->visible) {
      if (auto parent = get_parent()) {
        return parent->is_recursively_visible();
      }
      return true;
    }
    return false;
  }

public:
  virtual ~Component() {
  }

  template<typename T, typename ... Args>
  std::enable_if_t<is_component_v<T>, T&> add(Args ... args) noexcept (false) {
    add(std::make_shared<T>(std::forward<Args>(args)...));
  }

  void add(const std::shared_ptr<Component> &component) noexcept (false) {
    add(component, { });
  }

  void add(const std::shared_ptr<Component> &component, const std::any &constraints) noexcept (false) {
    add_impl(component, constraints);
  }

  /**
   * Removes the specified component from this container.
   */
  void remove(const std::shared_ptr<Component> &component) {
    remove_impl(component);
    if (this->focus_component.lock() == component) {
      this->focus_component.reset();
      this->focus_component = get_focus_component();
    }
    invalidate();
  }

  /**
   * Return a reference to the (non-container) component inside this Container that has the keyboard input focus (or would have it, if the
   * focus was inside this container). If no component inside the container has the focus, choose the first FocusTraversable component.
   *
   * @return the Component in this container that would have the focus; never null.
   * @throws IllegalComponentStateException
   *             if there is no focus-traversable component in this container.
   */
  std::shared_ptr<Component> get_focus_component() {
    if (not this->focus_component.lock() and has_children()) {
      for (auto &&c : this->components) {
        if (c->is_focusable()) {
          this->focus_component = c;
          break;
        }
      }
    }

    if (auto c = this->focus_component.lock()) {
      if (c->has_children()) {
        if (auto focus_component = c->get_focus_component()) {
          return focus_component;
        }
      }
    }
    return this->focus_component.lock();
  }

  std::shared_ptr<Component> get_parent() const {
    return this->parent.lock();
  }

  int get_x() const {
    return this->location.x;
  }

  int get_y() const {
    return this->location.y;
  }

  int get_width() const {
    return this->size.width;
  }

  int get_height() const {
    return this->size.height;
  }

  bool has_children() const {
    return not this->components.empty();
  }

  /**
   * Determines whether this component is displayable. A component is displayable when it is connected to a native screen resource.
   * <p>
   * A component is made displayable either when it is added to a displayable containment hierarchy or when its containment hierarchy is
   * made displayable. A containment hierarchy is made displayable when its ancestor window is either packed or made visible.
   * <p>
   * A component is made undisplayable either when it is removed from a displayable containment hierarchy or when its containment
   * hierarchy is made undisplayable. A containment hierarchy is made undisplayable when its ancestor window is disposed.
   *
   * @return <code>true</code> if the component is displayable, <code>false</code> otherwise
   */
  virtual bool is_displayable() const {
    // Every component that has been added to a Container has a parent.
    // The Window class overrides this method because it is never added to
    // a Container.
    if (auto parent = get_parent()) {
      return parent->is_displayable();
    }
    return false;
  }

  bool is_enabled() const {
    return this->enabled;
  }

  /**
   * Indicates whether this component can be traversed using Tab or Shift-Tab keyboard focus traversal. If this method returns "false" it
   * can still request focus using requestFocus(), but it will not automatically be assigned focus during keyboard focus traversal.
   */
  bool is_focusable() const {
    return is_enabled() and is_recursively_visible();
  }

  /**
   * Determines whether this component is showing on screen. This means that the component must be visible, and it must be in a container
   * that is visible and showing.
   *
   * @return <code>true</code> if the component is showing, <code>false</code> otherwise
   * @see #setVisible
   */
  bool is_showing() const {
    if (this->visible) {
      if (auto parent = this->parent.lock()) {
        return parent->is_showing();
      }
      return true;
    }
    return false;
  }

  bool is_valid() const {
    return this->valid;
  }

  bool is_visible() const {
    return this->visible;
  }

  /**
   * Marks the component and all parents above it as needing to be laid out again. This method is overridden by Container.
   */
  void invalidate() {
    this->valid = false;
    if (auto parent = this->parent.lock()) {
      parent->invalidate_if_valid();
    }
  }

  virtual void paint(Graphics &g) = 0;

  /**
   * Causes this component to be repainted as soon as possible (this is done by posting a RepaintEvent onto the system queue).
   */
  void repaint() {
    repaint(get_x(), get_y(), get_width(), get_height());
  }

  /**
   * Repaints the specified rectangle of this component within <code>tm</code> milliseconds.
   * <p>
   * If this component is a lightweight component, this method causes a call to this component's <code>paint</code> method. Otherwise,
   * this method causes a call to this component's <code>update</code> method.
   * <p>
   * <b>Note</b>: For more information on the paint mechanisms utilitized by AWT and Swing, including information on how to write the most
   * efficient painting code, see <a href="http://www.oracle.com/technetwork/java/painting-140037.html">Painting in AWT and Swing</a>.

   * @param x
   *            the <i>x</i> coordinate
   * @param y
   *            the <i>y</i> coordinate
   * @param width
   *            the width
   * @param height
   *            the height
   * @see #update(Graphics)
   */
  void repaint(int x, int y, int width, int height) {
    // Needs to be translated to parent coordinates since
    // a parent native container provides the actual repaint
    // services. Additionally, the request is restricted to
    // the bounds of the component.
    if (auto parent = this->parent.lock()) {
      if (x < 0) {
        width += x;
        x = 0;
      }
      if (y < 0) {
        height += y;
        y = 0;
      }

      int pwidth = (width > this->size.width) ? this->size.width : width;
      int pheight = (height > this->size.height) ? this->size.height : height;

      if (pwidth <= 0 or pheight <= 0) {
        return;
      }

      int px = this->location.x + x;
      int py = this->location.y + y;
      parent->repaint(px, py, pwidth, pheight);
    } else {
      if (is_visible() and width > 0 and height > 0) {
//        PaintEvent e = new PaintEvent(this, new Rectangle(x, y, width, height));
//        Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e);
      }
    }
  }

  void repaint(const Rectangle &rect) {
    repaint(rect.x, rect.y, rect.width, rect.height);
  }

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

  void set_location(const Point &at) {
    set_location(at.x, at.y);
  }

  void set_location(int x, int y) {
    set_bounds(x, y, this->size.width, this->size.height);
  }

  void set_size(const Dimension &to) {
    set_size(to.width, to.height);
  }

  void set_size(int width, int height) {
    set_bounds(this->location.x, this->location.y, width, height);
  }

  void set_bounds(int x, int y, int width, int height) {
    std::unique_lock lock(tree_mutex);

    auto resized = (this->size.width != width) or (this->size.height != height);
    auto moved = (this->location.x != x) or (this->location.y != y);
    if (not resized and not moved) {
      return;
    }

    auto old_x = this->location.x;
    auto old_y = this->location.y;
    auto old_width = this->size.width;
    auto old_height = this->size.height;

    this->location.x = x;
    this->location.y = y;
    this->size.width = width;
    this->size.height = height;

    if (resized) {
      invalidate();
    } else if (auto parent = this->parent.lock()) {
      parent->invalidate_if_valid();
    }

    repaint_parent_if_needed(old_x, old_y, old_width, old_height);
  }

  void set_layout(const std::shared_ptr<Layout> &layout) {
    if (this->layout != layout) {
      this->layout = layout;
      invalidate();
    }
  }
};

}

#include <tui++/Window.h>

namespace tui {

inline bool Component::is_window(const std::shared_ptr<Component> &component) {
  return std::dynamic_pointer_cast<Window>(component) != nullptr;
}

}
