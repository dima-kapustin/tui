#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <exception>
#include <unordered_set>

#include <tui++/Point.h>
#include <tui++/Color.h>
#include <tui++/Border.h>
#include <tui++/Cursor.h>
#include <tui++/Insets.h>
#include <tui++/Layout.h>
#include <tui++/Object.h>
#include <tui++/Screen.h>
#include <tui++/Dimension.h>
#include <tui++/Rectangle.h>
#include <tui++/ActionMap.h>
#include <tui++/KeyStroke.h>
#include <tui++/ComponentInputMap.h>
#include <tui++/ComponentOrientation.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/FocusTraversalPolicy.h>

#include <tui++/event/EventSource.h>

namespace tui {

class Window;
class Graphics;
class Component;
class EventQueue;

template<typename T>
constexpr bool is_component_v = std::is_base_of_v<Component, T>;

class Component: virtual public Object, public std::enable_shared_from_this<Component>, public EventSource<ComponentEvent, FocusEvent, HierarchyEvent, HierarchyBoundsEvent, KeyEvent, MouseEvent, MouseClickEvent, MouseMoveEvent, MouseOverEvent, MouseWheelEvent> {
  using base = EventSource<ComponentEvent, FocusEvent, HierarchyEvent, HierarchyBoundsEvent, KeyEvent, MouseEvent, MouseClickEvent, MouseMoveEvent, MouseOverEvent, MouseWheelEvent>;

protected:
  static std::recursive_mutex tree_mutex;

  EventTypeMask event_mask = EventTypeMask::NONE;

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

  Property<std::shared_ptr<FocusTraversalPolicy>> focus_traversal_policy { this, "focus_traversal_policy" };
  bool focus_traversal_policy_provider;

  /**
   * Indicates whether this Component is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot
   * leave it via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and
   * all of this Container's descendants that are not descendants of inferior focus cycle roots.
   */
  Property<bool> focus_cycle_root { this, "focus_cycle_root", false };
  Property<bool> opaque { this, "opaque" };

  Property<bool> focus_traversal_keys_enabled { this, "focus_traversal_keys_enabled" };
  std::vector<std::shared_ptr<const std::unordered_set<KeyStroke>>> focus_traversal_keys;

  /**
   * Indicates whether valid containers should also traverse their
   * children and call the validate_tree() method on them.
   */
  static bool descend_unconditionally_when_validating;

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
    if (not this->valid or descend_unconditionally_when_validating) {
      do_layout();
      for (auto &&c : this->components) {
        if (not c->is_valid() or descend_unconditionally_when_validating) {
          if (not is_window(c)) {
            c->validate_tree();
          } else {
            c->validate();
          }
        }
      }
    }
    this->valid = true;
  }

  void validate_unconditionally() {
    descend_unconditionally_when_validating = true;
    validate();
    descend_unconditionally_when_validating = false;
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

  std::shared_ptr<Component> find_traversal_root() const {
    // I potentially have two roots, myself and my root parent
    // If I am the current root, then use me
    // If none of my parents are roots, then use me
    // If my root parent is the current root, then use my root parent
    // If neither I nor my root parent is the current root, then
    // use my root parent (a guess)

    auto current_focus_cycle_root = KeyboardFocusManager::get_current_focus_cycle_root();
    std::shared_ptr<Component> root;

    if (current_focus_cycle_root == shared_from_this()) {
      root = current_focus_cycle_root;
    } else {
      root = get_focus_cycle_root_ancestor();
      if (not root) {
        root = const_cast<Component*>(this)->shared_from_this();
      }
    }

    if (root != current_focus_cycle_root) {
      KeyboardFocusManager::get_current_focus_cycle_root(root);
    }
    return root;
  }

  std::shared_ptr<Window> get_containing_window() const;

  bool is_request_focus_accepted(bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  bool is_parent_of(std::shared_ptr<Component> component) const;
  void clear_current_focus_cycle_root_on_hide();
  void clear_most_recent_focus_owner_on_hide();

  bool contains_focus() const;

protected:
  Component() {
  }

protected:
  static bool is_window(const std::shared_ptr<Component> &component);

  void enable_events(EventTypeMask event_mask);
  void disable_events(EventTypeMask event_mask);

  virtual void event_listener_mask_updated(const EventTypeMask &removed, const EventTypeMask &added) override;

  virtual void show();
  virtual void hide();

  void add_impl(const std::shared_ptr<Component> &c, const std::any &constraints) noexcept (false);

  virtual void add_notify();

  void remove_impl(const std::shared_ptr<Component> &component);

  virtual void remove_notify();

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

  virtual Screen* get_screen() const;

  EventQueue* get_event_queue() const;

  virtual void process_event(FocusEvent &e) override {
    base::process_event(e);
  }

  virtual void process_event(MouseEvent &e) override {
    base::process_event(e);
  }

  virtual void process_event(KeyEvent &e) override {
    base::process_event(e);
    if (not e.consumed) {
      e.consumed = process_key_bindings(e);
    }
  }

  virtual bool process_key_bindings(KeyEvent &e);

  virtual bool process_key_binding(const KeyStroke &ks, KeyEvent &e, InputCondition condition);

  static bool process_key_bindings_for_all_components(KeyEvent &e, const std::shared_ptr<Component> &container);
  static bool notify_action(const std::shared_ptr<Action> &action, const KeyStroke &ks, KeyEvent &e, const std::shared_ptr<Component> &sender);

  std::shared_ptr<Component> get_traversal_root() const {
    if (is_focus_cycle_root()) {
      return find_traversal_root();
    }

    return get_focus_cycle_root_ancestor();
  }

  bool can_be_focus_owner() const {
    // It is enabled, visible, focusable.
    if (is_enabled() and is_displayable() and is_visible() and is_focusable()) {
      return true;
    }
    return false;
  }

  std::shared_ptr<Component> get_next_focus_candidate() const;

  bool transfer_focus(bool clear_on_failure);
  bool transfer_focus_backward(bool clear_on_failure);

  bool request_focus(bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  bool request_focus(bool temporary, FocusEvent::Cause cause) {
    return request_focus(temporary, true, cause);
  }

  bool dispatch_mouse_wheel_to_ancestor(MouseWheelEvent &e);

  virtual void create_hierarchy_events(HierarchyEvent::Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent);
  virtual void create_hierarchy_bounds_events(HierarchyBoundsEvent::Type type, const std::shared_ptr<Component> &changed, const std::shared_ptr<Component> &changed_parent);

  template<typename T, typename ... Args>
  void post_event(Args &&... args) {
    if (is_event_enabled<T>()) {
      get_screen()->post<T>(shared_from_this(), std::forward<Args>(args)...);
    }
  }

  template<typename T, typename Component, typename ... Args>
  void post_event(Args &&... args) {
    if (is_event_enabled<T>()) {
      get_screen()->post<T>(std::dynamic_pointer_cast<Component>(shared_from_this()), std::forward<Args>(args)...);
    }
  }

  std::shared_ptr<Component> get_mouse_event_target(int x, int y, bool include_self) const;

  friend class Window;
  friend class KeyboardFocusManager;

public:
  virtual ~Component();

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  T& add(Args ... args) noexcept (false) {
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

  virtual void dispatch_event(Event &e);

  template<typename T, typename ... Args>
  void dispatch_event(Args &&... args) {
    auto e = Event { std::in_place_type<T>, shared_from_this(), std::forward<Args>(args)... };
    dispatch_event(e);
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
    for (auto &&c : this->components) {
      auto cx = c->get_x();
      auto cy = c->get_y();

      if (c->contains(x - cx, y - cy)) {
        return c;
      }
    }
    return {};
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

  void request_focus(FocusEvent::Cause cause) {
    request_focus(false, true, cause);
  }

  bool request_focus_in_window(FocusEvent::Cause cause) {
    return request_focus(false, false, cause);
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
    if (not this->valid or descend_unconditionally_when_validating) {
      validate_tree();
    }
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

  std::shared_ptr<InputMap> get_input_map(InputCondition condition) {
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

  bool is_focus_owner() const {
    return KeyboardFocusManager::get_focus_owner().get() == this;
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

  void transfer_focus() {
    transfer_focus(false);
  }

  void transfer_focus_backward() {
    transfer_focus_backward(false);
  }

  void transfer_focus_up_cycle();

  void transfer_focus_down_cycle();

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

  bool is_focus_traversal_policy_provider() const {
    return this->focus_traversal_policy_provider;
  }

  std::shared_ptr<FocusTraversalPolicy> get_focus_traversal_policy() const {
    if (not is_focus_traversal_policy_provider() and not is_focus_cycle_root()) {
      return {};
    } else if (this->focus_traversal_policy) {
      return this->focus_traversal_policy;
    } else if (auto root_ancestor = get_focus_cycle_root_ancestor()) {
      return root_ancestor->get_focus_traversal_policy();
    } else {
      return KeyboardFocusManager::get_default_focus_traversal_policy();
    }
  }

  void set_focus_traversal_policy(const std::shared_ptr<FocusTraversalPolicy> &policy) {
    // TODO synchronize?
    this->focus_traversal_policy = policy;
  }

  template<typename Event>
  bool is_event_enabled() const {
    return is_event_enabled(event_type_v<Event>);
  }

  bool is_event_enabled(EventType event_type) const {
    return (this->event_mask & event_type) or has_event_listeners(event_type);
  }

  void set_focus_traversal_keys_enabled(bool focus_traversal_keys_enabled) {
    this->focus_traversal_keys_enabled = focus_traversal_keys_enabled;
  }

  /**
   * Returns whether focus traversal keys are enabled for this Component.
   * Components for which focus traversal keys are disabled receive key
   * events for focus traversal keys. Components for which focus traversal
   * keys are enabled do not see these events; instead, the events are
   * automatically converted to traversal operations.
   */
  bool get_focus_traversal_keys_enabled() const {
    return focus_traversal_keys_enabled;
  }

  std::shared_ptr<const std::unordered_set<KeyStroke>> get_focus_traversal_keys(KeyboardFocusManager::FocusTraversalKeys id) const;
  void set_focus_traversal_keys(KeyboardFocusManager::FocusTraversalKeys id, const std::shared_ptr<const std::unordered_set<KeyStroke>> &keyStrokes);
};

template<typename BaseComponent, typename ... Events>
using ComponentExtension = EventSourceExtension<BaseComponent, Events...>;

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
