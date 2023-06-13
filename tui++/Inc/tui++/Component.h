#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <exception>

#include <tui++/Point.h>
#include <tui++/Color.h>
#include <tui++/Border.h>
#include <tui++/Cursor.h>
#include <tui++/Insets.h>
#include <tui++/Layout.h>
#include <tui++/Dimension.h>
#include <tui++/Rectangle.h>
#include <tui++/ActionMap.h>
#include <tui++/ComponentInputMap.h>
#include <tui++/ComponentOrientation.h>

#include <tui++/event/EventSource.h>

#include <tui++/Object.h>

namespace tui {

class Screen;
class Window;
class Graphics;
class Component;
class EventQueue;

template<typename T>
constexpr bool is_component_v = std::is_base_of_v<Component, T>;

class Component: public std::enable_shared_from_this<Component>, virtual public Object, public BasicEventSource<FocusEvent, MouseEvent, KeyEvent> {
  using EventSource = BasicEventSource<FocusEvent, MouseEvent, KeyEvent>;

  static std::recursive_mutex tree_mutex;

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
  mutable std::weak_ptr<Component> focus_component;

  struct {
    int was_focus_owner :1 = false;
  };

  float alignment_x = CENTER_ALIGNMENT;
  float alignment_y = CENTER_ALIGNMENT;

  ComponentOrientation orientation = ComponentOrientation::UNKNOWN;

  Point location { };
  Dimension size { };

  mutable std::optional<Dimension> minimum_size;
  mutable Property<std::optional<Dimension>> preferred_size { this, "preferred_size" };

  /**
   * A flag that is set to true when the container is laid out, and set to false when a component is added or removed from the container
   * (indicating that it needs to be laid out again).
   */
  bool valid = false;

  Property<bool> enabled { this, "enabled" };
  Property<bool> visible { this, "visible" };
  Property<std::optional<Cursor>> cursor { this, "cursor" };
  Property<std::optional<Color>> background_color { this, "background_color" };
  Property<std::optional<Color>> foreground_color { this, "foreground_color" };

  mutable std::shared_ptr<InputMap> focus_input_map;
  mutable std::shared_ptr<InputMap> ancestor_input_map;
  mutable std::shared_ptr<ComponentInputMap> window_input_map;
  mutable std::shared_ptr<ActionMap> action_map;

  /**
   * Indicates whether this Component is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot
   * leave it via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and
   * all of this Container's descendants that are not descendants of inferior focus cycle roots.
   */
  Property<bool> focus_cycle_root { this, "focus_cycle_root", false };
  Property<bool> opaque { this, "opaque" };

public:
  constexpr static float TOP_ALIGNMENT = 0;
  constexpr static float BOTTOM_ALIGNMENT = 1.0;
  constexpr static float CENTER_ALIGNMENT = 0.5;
  constexpr static float LEFT_ALIGNMENT = 0;
  constexpr static float RIGHT_ALIGNMENT = 1.0;

  enum InputCondition {
    // Command should be invoked when the component has the focus.
    WHEN_FOCUSED = 0,
    // Command should be invoked when the receiving component is an ancestor of the focused component or is itself the focused component.
    WHEN_ANCESTOR_OF_FOCUSED_COMPONENT = 1,
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

  void validate_tree() {
    if (not this->valid) {
      do_layout();
      for (auto &&c : this->components) {
        if (not c->is_valid()) {
          if (not is_window(c)) {
            c->validate_tree();
          } else {
            c->validate();
          }
        }
      }
    }
  }

  std::shared_ptr<InputMap> get_input_map(InputCondition condition, bool create) const {
    switch (condition) {
    case WHEN_FOCUSED:
      if (not this->focus_input_map and create) {
        this->focus_input_map = std::make_shared<InputMap>();
      }
      return this->focus_input_map;
    case WHEN_ANCESTOR_OF_FOCUSED_COMPONENT:
      if (not this->ancestor_input_map and create) {
        this->ancestor_input_map = std::make_shared<InputMap>();
      }
      return this->ancestor_input_map;
    case WHEN_IN_FOCUSED_WINDOW:
      if (not this->window_input_map and create) {
        this->window_input_map = std::make_shared<ComponentInputMap>(const_cast<Component*>(this)->shared_from_this());
      }
      return this->window_input_map;
    }
    return {};
  }

  /**
   * Sets the keyboard focus to the first component that is focusTraversable. Called by the nextFocus() method when it runs out of
   * components in the current container to move the focus to. The nextFocus() method first checks that this container contains a
   * focusTraversable component before calling this.
   */
  void first_focus() {
    if (has_children()) {
      for (auto &&c : this->components) {
        if (c->is_focusable()) {
          c->first_focus();
          this->focus_component = c;
          return;
        }
      }
    }
  }

  /**
   * Sets the keyboard focus to the last component that is focusTraversable. Called by the previousFocus() method when it runs out of
   * components in the current container to move the focus to. The previousFocus() method first checks that this container contains a
   * focusTraversable component before calling this.
   */
  void last_focus() {
    if (has_children()) {
      for (int i = this->components.size() - 1; i >= 0; i--) {
        auto &&c = this->components[i];
        if (c->is_focusable()) {
          c->last_focus();
          this->focus_component = c;
          return;
        }
      }
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

  /**
   * Causes this container to lay out its components. Most programs should not call this method directly, but should invoke the
   * <code>validate</code> method instead.
   */
  virtual void do_layout() {
    if (this->layout) {
      this->layout->layout(shared_from_this());
    } else if (has_children()) {
      for (auto &&c : this->components) {
        c->set_size(c->get_preferred_size());
      }
    }
  }

  void paint_border(Graphics &g);

  virtual void paint_components(Graphics &g);

  /**
   * Set this container's current keyboard focus. Called by the requestFocus() method of the contained component.
   */
  void set_focus(std::shared_ptr<Component> focus) {
    this->focus_component = focus;
    if (auto parent = get_parent()) {
      parent->set_focus(shared_from_this());
    }
  }

  std::shared_ptr<Component> get_component_at(int x, int y, bool visible_only) const {
    for (auto &&c : this->components) {
      auto cx = c->get_x();
      auto cy = c->get_y();

      if ((not visible_only or c->is_visible()) and c->contains(x - cx, y - cy)) {
        if (c->has_children()) {
          return c->get_component_at(x - cx, y - cy, visible_only);
        }
        return c;
      }
    }
    return {};
  }

  virtual Screen* get_screen() const;

  std::shared_ptr<Window> get_top_window() const;
  EventQueue& get_event_queue() const;

  virtual void process_event(FocusEvent &e) override {
    EventSource::process_event(e);
  }

  virtual void process_event(MouseEvent &e) override {
    EventSource::process_event(e);
  }

  virtual void process_event(KeyEvent &e) override {
    EventSource::process_event(e);
    if (not e.consumed) {
      e.consumed = process_key_bindings(e);
    }
  }

  virtual bool process_key_bindings(KeyEvent &e);

  virtual bool process_key_binding(const KeyStroke &ks, KeyEvent &e, InputCondition condition);

  static bool process_key_bindings_for_all_components(KeyEvent &e, const std::shared_ptr<Component> &container);

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

  bool contains(int x, int y) const {
    return (x >= 0 and x < get_width() and y >= 0 and y < get_height());
  }

  bool contains(const Point &p) const {
    return contains(p.x, p.y);
  }

  std::shared_ptr<Border> get_border() const {
    return this->border;
  }

  ComponentOrientation get_component_orientation() const {
    return this->orientation;
  }

  std::shared_ptr<Component> get_component_at(int x, int y) const {
    return get_component_at(x, y, false);
  }

  std::shared_ptr<Component> get_component_at(const Point &p) const {
    return get_component_at(p.x, p.y);
  }

  size_t get_component_count() const {
    return this->components.size();
  }

  const std::shared_ptr<Component>& get_component(size_t index) const {
    return this->components[index];
  }

  size_t get_component_index(const std::shared_ptr<const Component> &component) const {
    return get_component_index(component.get());
  }

  size_t get_component_index(const Component *component) const {
    for (size_t index = 0; index != this->components.size(); ++index) {
      if (this->components[index].get() == component) {
        return index;
      }
    }
    return size_t(-1);
  }

  /**
   * Return a reference to the (non-container) component inside this Container that has the keyboard input focus (or would have it, if the
   * focus was inside this container). If no component inside the container has the focus, choose the first FocusTraversable component.
   *
   * @return the Component in this container that would have the focus; never nullptr.
   */
  std::shared_ptr<Component> get_focus_component() const {
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

  Insets get_insets() const {
    if (this->border) {
      return this->border->get_border_insets(*this);
    } else {
      return {0, 0, 0, 0};
    }
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

  int get_height() const {
    return this->size.height;
  }

  int get_width() const {
    return this->size.width;
  }

  const Point& get_location() const {
    return this->location;
  }

  const Dimension& get_size() const {
    return this->size;
  }

  /**
   * Get the Window that contains this component.
   */
  std::shared_ptr<Window> get_window_ancestor() const noexcept (false);

  /**
   * Gets the preferred size of this component.
   *
   * @return a dimension object indicating this component's preferred size
   */
  Dimension get_preferred_size() const {
    if (not this->valid and not this->preferred_size.has_vlaue()) {
      std::unique_lock lock(tree_mutex);
      if (this->layout) {
        this->preferred_size = this->layout->get_preferred_layout_size(shared_from_this());
      } else {
        this->preferred_size = this->minimum_size;
      }
    }
    return this->preferred_size.value_or(Dimension { });
  }

  Dimension get_minimum_size() const {
    if (not this->layout) {
      return this->size;
    } else if (not this->valid or not this->minimum_size.has_value()) {
      this->minimum_size = this->layout->get_minimum_layout_size(shared_from_this());
    }
    return this->minimum_size.value_or(Dimension { });
  }

  float get_alignment_x() const {
    if (this->layout) {
      std::unique_lock lock(tree_mutex);
      return this->layout->get_layout_alignment_x(shared_from_this());
    } else {
      return this->alignment_x;
    }
  }

  /**
   * Returns the alignment along the y axis. This specifies how the component would like to be aligned relative to other components. The
   * value should be a number between 0 and 1 where 0 represents alignment along the origin, 1 is aligned the furthest away from the
   * origin, 0.5 is centered, etc.
   */
  float get_alignment_y() {
    if (this->layout) {
      std::unique_lock lock(tree_mutex);
      return this->layout->get_layout_alignment_y(shared_from_this());
    } else {
      return this->alignment_y;
    }
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

  virtual void paint(Graphics &g) {
    paint_border(g);
    paint_components(g);
  }

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

  void request_focus();

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

  /**
   * Sets the preferred size of this component to a constant value. Subsequent calls to <code>getPreferredSize</code> will always return
   * this value. Setting the preferred size to <code>null</code> restores the default behavior.
   */
  void set_preferred_size(const Dimension &preferred_size) {
    this->preferred_size = preferred_size;
  }

  /**
   * Ensures that this component is laid out correctly. This method is primarily intended to be used on instances of Container. The
   * default implementation does nothing; it is overridden by Container.
   */
  void validate() {
    validate_tree();
    this->valid = true;
  }

  std::optional<Color> get_background_color() const {
    if (not this->background_color.has_vlaue()) {
      if (auto parent = get_parent()) {
        return parent->get_background_color();
      }
    }
    return this->background_color;
  }

  void set_background_color(std::optional<Color> color) {
    this->background_color = color;
  }

  std::optional<Color> get_foreground_color() const {
    if (not this->foreground_color.has_vlaue()) {
      if (auto parent = get_parent()) {
        return parent->get_foreground_color();
      }
    }
    return this->foreground_color;
  }

  void set_foreground_color(std::optional<Color> color) {
    this->foreground_color = color;
  }

  std::optional<Cursor> get_cursor() const {
    if (not this->cursor.has_vlaue()) {
      if (auto parent = get_parent()) {
        return parent->get_cursor();
      }
    }
    return this->cursor;
  }

  void set_cursor(const std::optional<Cursor> &cursor) {
    this->cursor = cursor;
  }

  std::shared_ptr<ActionMap> get_action_map() const {
    if (not this->action_map) {
      this->action_map = std::make_shared<ActionMap>();
    }
    return this->action_map;
  }

  void set_action_map(const std::shared_ptr<ActionMap> &action_map) {
    this->action_map = action_map;
  }

  std::shared_ptr<InputMap> get_input_map() const {
    return get_input_map(WHEN_FOCUSED, true);
  }

  std::shared_ptr<InputMap> getInputMap(InputCondition condition) {
    return get_input_map(condition, true);
  }

  void set_input_map(InputCondition condition, const std::shared_ptr<InputMap> &map) {
    switch (condition) {
    case WHEN_IN_FOCUSED_WINDOW:
      if (auto component_input_map = std::dynamic_pointer_cast<ComponentInputMap>(map)) {
        this->window_input_map = component_input_map;
      } else {
        throw std::runtime_error("WHEN_IN_FOCUSED_WINDOW InputMaps must be of type ComponentInputMap");
      }
      //registerWithKeyboardManager(false);
      break;
    case WHEN_ANCESTOR_OF_FOCUSED_COMPONENT:
      this->ancestor_input_map = map;
      break;
    case WHEN_FOCUSED:
      this->focus_input_map = map;
      break;
    }
  }

  auto begin() -> decltype(this->components.begin()) {
    return this->components.begin();
  }

  auto end() -> decltype(this->components.end()) {
    return this->components.end();
  }

  auto begin() const -> decltype(this->components.begin()) {
    return this->components.begin();
  }

  auto end() const -> decltype(this->components.end()) {
    return this->components.end();
  }

  auto get_tree_lock() const -> decltype(std::unique_lock {tree_mutex}) {
    return std::unique_lock { tree_mutex };
  }

  template<typename Callable>
  auto with_tree_locked(Callable &&callable) const -> decltype(callable()) {
    auto lock = get_tree_lock();
    return callable();
  }

  /**
   * Returns whether this Container is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot leave
   * it via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and all of
   * this Container's descendants that are not descendants of inferior focus cycle roots. Note that a FocusTraversalPolicy may bend these
   * restrictions, however. For example, ContainerOrderFocusTraversalPolicy supports implicit down-cycle traversal.
   */
  bool is_focus_cycle_root() const {
    return this->focus_cycle_root;
  }

  /**
   * Sets whether this Container is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot leave it
   * via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and all of
   * this Container's descendants that are not descendants of inferior focus cycle roots.
   */
  void set_focus_cycle_root(bool focus_cycle_root) {
    this->focus_cycle_root = focus_cycle_root;
  }

  bool is_focus_cycle_root(const Component *component) const {
    if (is_focus_cycle_root() and component == this) {
      return true;
    } else {
      return get_focus_cycle_root_ancestor().get() == component;
    }
  }

  std::shared_ptr<Component> get_focus_cycle_root_ancestor() const {
    auto root_ancestor = get_parent();
    while (root_ancestor and not root_ancestor->is_focus_cycle_root()) {
      root_ancestor = root_ancestor->get_parent();
    }
    return root_ancestor;
  }

  void transfer_focus();
  void transfer_focus_backward();
  void transfer_focus_up_cycle();

  /**
   * Returns true if this component is completely opaque.
   * <p>
   * An opaque component paints every pixel within its
   * rectangular bounds. A non-opaque component paints only a subset of
   * its pixels or none at all, allowing the pixels underneath it to
   * "show through".  Therefore, a component that does not fully paint
   * its pixels provides a degree of transparency.
   * <p>
   * Subclasses that guarantee to always completely paint their contents
   * should override this method and return true.
   *
   * @return true if this component is completely opaque
   * @see #setOpaque
   */
  bool is_opaque() const {
    return this->opaque;
  }

  /**
   * If true the component paints every pixel within its bounds.
   * Otherwise, the component may not paint some or all of its
   * pixels, allowing the underlying pixels to show through.
   * <p>
   * The default value of this property is false for <code>JComponent</code>.
   * However, the default value for this property on most standard
   * <code>JComponent</code> subclasses (such as <code>JButton</code> and
   * <code>JTree</code>) is look-and-feel dependent.
   */
  void set_opaque(bool value) {
    this->opaque = value;
  }
};

/**
 * Convert a point from a screen coordinates to a component's coordinate system
 */
Point convert_point_from_screen(int x, int y, std::shared_ptr<Component> to);
inline Point convert_point_from_screen(const Point &p, std::shared_ptr<Component> to) {
  return convert_point_from_screen(p.x, p.y, to);
}

/**
 * Convert a point from a component's coordinate system to screen coordinates.
 */
Point convert_point_to_screen(int x, int y, std::shared_ptr<Component> from);
inline Point convert_point_to_screen(const Point &p, std::shared_ptr<Component> from) {
  return convert_point_to_screen(p.x, p.y, from);
}

}

#include <tui++/Window.h>

namespace tui {

inline bool Component::is_window(const std::shared_ptr<Component> &component) {
  return std::dynamic_pointer_cast<Window>(component) != nullptr;
}

}
