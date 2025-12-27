#pragma once

#include <mutex>
#include <memory>
#include <vector>
#include <concepts>
#include <exception>
#include <unordered_set>
#include <unordered_map>

#include <tui++/Point.h>
#include <tui++/Color.h>
#include <tui++/Border.h>
#include <tui++/Cursor.h>
#include <tui++/Insets.h>
#include <tui++/Layout.h>
#include <tui++/Object.h>
#include <tui++/Dimension.h>
#include <tui++/Rectangle.h>
#include <tui++/ActionMap.h>
#include <tui++/KeyStroke.h>
#include <tui++/Constraints.h>
#include <tui++/ComponentInputMap.h>
#include <tui++/ComponentOrientation.h>
#include <tui++/KeyboardFocusManager.h>
#include <tui++/FocusTraversalPolicy.h>

#include <tui++/event/EventSource.h>

#include <tui++/lookandfeel/ComponentUI.h>

namespace tui {

class Window;
class Graphics;
class Component;
class PopupMenu;
class EventQueue;
class KeyboardManager;

template<typename T>
constexpr bool is_component_v = std::is_base_of_v<Component, T>;

class Component: public Object, public std::enable_shared_from_this<Component>, public EventSource<ComponentEvent, ContainerEvent, FocusEvent, HierarchyEvent, HierarchyBoundsEvent, KeyEvent, MousePressEvent, MouseClickEvent, MouseMoveEvent, MouseOverEvent, MouseWheelEvent> {
  using base = EventSource<ComponentEvent, ContainerEvent, FocusEvent, HierarchyEvent, HierarchyBoundsEvent, KeyEvent, MousePressEvent, MouseClickEvent, MouseMoveEvent, MouseOverEvent, MouseWheelEvent>;

protected:
  static std::recursive_mutex tree_mutex;

  /**
   * The event_mask is ONLY set by subclasses via enable_events().
   * The mask should NOT be set when listeners are registered
   * so that we can distinguish the difference between when
   * listeners request events and subclasses request them.
   */
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
    int inherits_popup_menu :1 = false;
  };

  float alignment_x = CENTER_ALIGNMENT;
  float alignment_y = CENTER_ALIGNMENT;

  ComponentOrientation orientation = ComponentOrientation::UNKNOWN;

  Point location { };
  Dimension size { };

  mutable Property<std::optional<Dimension>> minimum_size { this, "minimum-size" };
  mutable Property<std::optional<Dimension>> maximum_size { this, "maximum-size" };
  mutable Property<std::optional<Dimension>> preferred_size { this, "preferred-size" };

  struct {
    unsigned is_valid :1;
    unsigned is_alignment_x_set :1;
    unsigned is_alignment_y_set :1;
    unsigned is_focus_traversable_overridden :1;
  } flags;

  Property<bool> enabled { this, "enabled", true };
  Property<bool> visible { this, "visible", true };
  Property<bool> focusable { this, "focusable", true };
  Property<std::optional<Cursor>> cursor { this, "cursor" };
  Property<std::optional<Color>> background_color { this, "background-color" };
  Property<std::optional<Color>> foreground_color { this, "foreground-color" };

  mutable std::shared_ptr<InputMap> focus_input_map;
  mutable std::shared_ptr<InputMap> ancestor_input_map;
  mutable std::shared_ptr<ComponentInputMap> window_input_map;
  mutable std::shared_ptr<ActionMap> action_map;

  Property<std::shared_ptr<FocusTraversalPolicy>> focus_traversal_policy { this, "focus-traversal-policy" };
  bool focus_traversal_policy_provider = false;

  /**
   * Indicates whether this Component is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot
   * leave it via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and
   * all of this Container's descendants that are not descendants of inferior focus cycle roots.
   */
  Property<bool> focus_cycle_root { this, "focus-cycle-root", false };
  Property<bool> opaque { this, "opaque" };

  Property<bool> focus_traversal_keys_enabled { this, "focus-traversal-keys-enabled" };
  std::vector<std::shared_ptr<const std::unordered_set<KeyStroke>>> focus_traversal_keys;

  /**
   * Indicates whether valid containers should also traverse their
   * children and call the validate_tree() method on them.
   */
  static bool descend_unconditionally_when_validating;

  std::unordered_map<std::string_view, PropertyValue> client_properties;

  Property<std::shared_ptr<PopupMenu>> component_popup_menu { this, "component-popup-menu" };
  Property<std::string> tool_tip_text { this, "tool-tip-text" };

  Property<std::shared_ptr<laf::ComponentUI>> ui { this, "ui" };

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
    if (not this->flags.is_valid or descend_unconditionally_when_validating) {
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
    this->flags.is_valid = true;
  }

  void validate_unconditionally() {
    descend_unconditionally_when_validating = true;
    validate();
    descend_unconditionally_when_validating = false;
  }

  std::shared_ptr<InputMap> get_input_map(InputCondition condition, bool create) const;

  std::shared_ptr<Component> find_traversal_root() const;

  bool is_request_focus_accepted(bool temporary, bool focused_window_change_allowed, FocusEvent::Cause cause);

  bool is_parent_of(std::shared_ptr<Component> component) const;
  void clear_current_focus_cycle_root_on_hide();
  void clear_most_recent_focus_owner_on_hide();

  bool contains_focus() const;

  void set_ui(std::shared_ptr<laf::ComponentUI> const &ui);

protected:
  Component() {
  }

  virtual void init();

  template<typename T, typename ... Args>
  requires (is_component_v<T> )
  friend auto make_component(Args&&...);

protected:
  static bool is_window(const std::shared_ptr<const Component> &component);

  std::shared_ptr<Window> get_containing_window() const;

  constexpr static auto npos = std::numeric_limits<size_t>::max();

  size_t get_component_index(const Component *component) const {
    for (auto index = 0U; index != this->components.size(); ++index) {
      if (this->components[index].get() == component) {
        return index;
      }
    }
    return npos;
  }

  void enable_events(EventTypeMask event_mask);
  void disable_events(EventTypeMask event_mask);

  virtual void event_listener_mask_updated(const EventTypeMask &removed, const EventTypeMask &added) override;

  virtual void show();
  virtual void hide();

  void assert_adding_none_window(const std::shared_ptr<const Component> &c) noexcept (false);
  void assert_adding_none_cyclic(const std::shared_ptr<const Component> &c) noexcept (false);

  virtual void add_impl(const std::shared_ptr<Component> &c, const Constraints &constraints, int z_order) noexcept (false);

  virtual void add_notify();
  virtual void remove_notify();

  void add_delicately(const std::shared_ptr<Component> &c, const std::shared_ptr<Component> &parent, int z_order);
  bool remove_delicately(const std::shared_ptr<Component> &c, const std::shared_ptr<Component> &new_parent, int new_z_order);

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
    }
  }

  virtual void paint_component(Graphics &g);
  virtual void paint_border(Graphics &g);
  virtual void paint_children(Graphics &g);

  /**
   * Set this container's current keyboard focus. Called by the requestFocus() method of the contained component.
   */
  void set_focus(std::shared_ptr<Component> focus) {
    this->focus_component = focus;
    if (auto parent = get_parent()) {
      parent->set_focus(shared_from_this());
    }
  }

  virtual void process_event(FocusEvent &e) override {
    base::process_event(e);
  }

  virtual void process_event(MousePressEvent &e) override {
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

  static bool process_key_bindings_for_all_components(KeyEvent &e, std::shared_ptr<Component> container);
  static bool notify_action(const std::shared_ptr<Action> &action, const KeyStroke &ks, KeyEvent &e, const std::shared_ptr<Component> &sender);

  std::shared_ptr<Component> get_traversal_root() const {
    if (is_focus_cycle_root()) {
      return find_traversal_root();
    }

    return get_focus_cycle_root_ancestor();
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

  std::shared_ptr<Component> get_mouse_event_target(int x, int y, bool include_self) const;

  bool can_be_focus_owner_recursively();
  bool can_contain_focus_owner(const std::shared_ptr<Component> &candidate);

  virtual std::shared_ptr<laf::ComponentUI> create_ui() {
    return {};
  }

  virtual bool always_on_top() const {
    return false;
  }

  friend class Window;
  friend class KeyboardFocusManager;
  friend class KeyboardManager;

public:
  virtual ~Component();

  std::string get_name() const {
    return this->name;
  }

  void set_name(std::string &name) {
    this->name = name;
  }

  void add(const std::shared_ptr<Component> &component) noexcept (false) {
    add(component, {});
  }

  void add(const std::shared_ptr<Component> &component, int index) noexcept (false) {
    add(component, {}, index);
  }

  void add(const std::shared_ptr<Component> &component, const Constraints &constraints) noexcept (false) {
    add(component, constraints, -1);
  }

  void add(const std::shared_ptr<Component> &component, const Constraints &constraints, int index) noexcept (false) {
    add_impl(component, constraints, index);
  }

  /**
   * Removes the specified component from this container.
   */
  virtual void remove(const std::shared_ptr<Component> &c);
  virtual void remove(size_t index);

  virtual void dispatch_event(Event &e);

  template<typename T, typename ... Args>
  void dispatch_event(Args &&... args) {
    auto e = make_event<T>(shared_from_this(), std::forward<Args>(args)...);
    dispatch_event(e);
  }

  bool contains(int x, int y) const {
    return this->ui ? this->ui->contains(shared_from_this(), x, y) : (x >= 0 and x < get_width() and y >= 0 and y < get_height());
  }

  bool contains(const Point &p) const {
    return contains(p.x, p.y);
  }

  std::shared_ptr<Border> get_border() const {
    return this->border;
  }

  void set_border(std::shared_ptr<Border> const& border) {
    this->border = border;
  }

  ComponentOrientation get_component_orientation() const {
    return this->orientation;
  }

  std::vector<std::shared_ptr<Component>> const& get_components() const {
    return this->components;
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

  int get_component_z_order(const std::shared_ptr<const Component> &c) const;
  void set_component_z_order(const std::shared_ptr<Component> &c, int new_z_order);

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

  template<typename T, std::enable_if_t<std::derived_from<T, Component>, bool> = true>
  std::shared_ptr<T> get_parent() const {
    for (auto parent = get_parent(); parent; parent = parent->get_parent()) {
      if (auto candidate = std::dynamic_pointer_cast<T>(parent)) {
        return candidate;
      }
    }
    return {};
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

  Point get_location_on_screen() const;

  const Dimension& get_size() const {
    return this->size;
  }

  Rectangle get_bounds() const {
    return {this->location, this->size};
  }

  /**
   * Gets the preferred size of this component.
   *
   * @return a dimension object indicating this component's preferred size
   */
  Dimension get_preferred_size() const {
    if (not this->flags.is_valid and not this->preferred_size.has_value()) {
      std::unique_lock lock(tree_mutex);
      if (this->layout) {
        this->preferred_size = this->layout->get_preferred_layout_size(shared_from_this());
      } else {
        this->preferred_size = this->minimum_size;
      }
    }
    return this->preferred_size.value_or(Dimension {});
  }

  Dimension get_minimum_size() const {
    if (not this->layout) {
      return this->size;
    } else if (not this->flags.is_valid or not this->minimum_size.has_value()) {
      this->minimum_size = this->layout->get_minimum_layout_size(shared_from_this());
    }
    return this->minimum_size.value_or(Dimension {});
  }

  Dimension get_maximum_size() const {
    if (not this->layout) {
      return this->size;
    } else if (not this->flags.is_valid or not this->maximum_size.has_value()) {
      this->maximum_size = this->layout->get_maximum_layout_size(shared_from_this());
    }
    return this->maximum_size.value_or(Dimension {});
  }

  float get_alignment_x() const {
    if (not this->flags.is_alignment_x_set) {
      if (this->layout) {
        std::unique_lock lock(tree_mutex);
        return this->layout->get_layout_alignment_x(shared_from_this());
      }
    }
    return this->alignment_x;
  }

  static float to_valid_alignment(float value) {
    return value > 1.0f ? 1.0f : value < 0.0f ? 0.0f : value;
  }

  void set_alignment_x(float value) {
    this->alignment_x = to_valid_alignment(value);
    this->flags.is_alignment_x_set = true;
  }

  float get_alignment_y() {
    if (not this->flags.is_alignment_y_set) {
      if (this->layout) {
        std::unique_lock lock(tree_mutex);
        return this->layout->get_layout_alignment_y(shared_from_this());
      }
    }
    return this->alignment_y;
  }

  void set_alignment_y(float value) {
    this->alignment_y = to_valid_alignment(value);
    this->flags.is_alignment_y_set = true;
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

  virtual void set_enabled(bool value) {
    auto old_enabled = this->enabled.value();
    this->enabled = value;
    if (old_enabled != this->enabled.value()) {
      repaint();
    }
  }

  bool is_focusable() const {
    return this->focusable;
  }

  void set_focusable(bool value);

  bool is_showing() const;

  bool is_valid() const {
    return this->flags.is_valid;
  }

  virtual bool is_validate_root() const {
    return false;
  }

  bool is_visible() const {
    return this->visible;
  }

  virtual void paint(Graphics &g);

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
        // TODO
//        PaintEvent e = new PaintEvent(this, new Rectangle(x, y, width, height));
//        Toolkit.getDefaultToolkit().getSystemEventQueue().postEvent(e);
      }
    }
  }

  void repaint(const Rectangle &rect) {
    repaint(rect.x, rect.y, rect.width, rect.height);
  }

  virtual void request_focus(FocusEvent::Cause cause) {
    request_focus(false, true, cause);
  }

  virtual bool request_focus_in_window(FocusEvent::Cause cause) {
    return request_focus(false, false, cause);
  }

  virtual void set_visible(bool value);

  void set_location(const Point &at) {
    set_location(at.x, at.y);
  }

  virtual void set_location(int x, int y) {
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

  std::shared_ptr<Layout> const& get_layout() const {
    return this->layout;
  }

  virtual void set_layout(const std::shared_ptr<Layout> &layout) {
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

  void invalidate() {
    this->flags.is_valid = false;

    if (not is_validate_root()) {
      if (auto parent = this->parent.lock()) {
        parent->invalidate_if_valid();
      }
    }
  }

  void validate() {
    if (not this->flags.is_valid or descend_unconditionally_when_validating) {
      validate_tree();
    }
  }

  void revalidate();

  std::optional<Color> const& get_background_color() const {
    if (not this->background_color.has_value()) {
      if (auto parent = get_parent()) {
        return parent->get_background_color();
      }
    }
    return this->background_color;
  }

  void set_background_color(std::optional<Color> const& color) {
    this->background_color = color;
  }

  std::optional<Color> const& get_foreground_color() const {
    if (not this->foreground_color.has_value()) {
      if (auto parent = get_parent()) {
        return parent->get_foreground_color();
      }
    }
    return this->foreground_color;
  }

  void set_foreground_color(std::optional<Color>const& color) {
    this->foreground_color = color;
  }

  std::optional<Cursor> get_cursor() const {
    if (not this->cursor.has_value()) {
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
    return std::unique_lock {tree_mutex};
  }

  template<typename Callable>
  auto with_tree_locked(Callable &&callable) const -> std::invoke_result_t<Callable> {
    auto lock = get_tree_lock();
    return callable();
  }

  bool can_be_focus_owner() const {
    // It is enabled, visible, focusable.
    if (is_enabled() and is_displayable() and is_visible() and is_focusable()) {
      return true;
    }
    return false;
  }

  bool is_focus_owner() const {
    return KeyboardFocusManager::single->get_focus_owner().get() == this;
  }

  /**
   * Returns whether this Container is the root of a focus traversal cycle. Once focus enters a traversal cycle, typically it cannot leave
   * it via focus traversal unless one of the up- or down-cycle keys is pressed. Normal traversal is limited to this Container, and all of
   * this Container's descendants that are not descendants of inferior focus cycle roots. Note that a FocusTraversalPolicy may bend these
   * restrictions, however. For example, ContainerOrderFocusTraversalPolicy supports implicit down-cycle traversal.
   */
  virtual bool is_focus_cycle_root() const {
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
  virtual bool is_opaque() const {
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
      return KeyboardFocusManager::single->get_default_focus_traversal_policy();
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

  bool is_event_enabled(Event &e) const {
    return is_event_enabled(e.id.type);
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

  /**
   * Returns whether the specified Container is the focus cycle root of this
   * Component's focus traversal cycle. Each focus traversal cycle has only
   * a single focus cycle root and each Component which is not a Container
   * belongs to only a single focus traversal cycle.
   */
  bool is_focus_cycle_root(const std::shared_ptr<const Component> &container) const {
    return (get_focus_cycle_root_ancestor() == container);
  }

  template<typename ValueType>
  const ValueType* get_client_property(const char *property_name) const {
    if (auto pos = this->client_properties.find(property_name); pos != this->client_properties.end()) {
      if (auto *value = std::any_cast<ValueType>(&pos->second)) {
        return value;
      }
    }
    return nullptr;
  }

  template<typename ValueType>
  void set_client_property(const char *property_name, const ValueType &value) {
    this->client_properties[property_name].emplace<ValueType>(value);
  }

  virtual void update_ui() {
    set_ui(create_ui());
  }

  std::shared_ptr<PopupMenu> get_component_popup_menu() const {
    if (not this->component_popup_menu and this->inherits_popup_menu) {
      if (auto parent = get_parent()) {
        return parent->get_component_popup_menu();
      }
    }
    return this->component_popup_menu;
  }

  void set_component_popup_menu(const std::shared_ptr<PopupMenu> &popup_menu) {
    if (popup_menu) {
      enable_events(MOUSE_EVENT_MASK);
    }
    this->component_popup_menu = popup_menu;
  }

  std::string const& get_tool_tip_text() const {
    return this->tool_tip_text;
  }

  void set_tool_tip_text(std::string const &tool_tip_text);
};

template<typename BaseComponent, typename ... Events>
using ComponentExtension = EventSourceExtension<BaseComponent, Events...>;

template<typename T, typename ... Args>
requires (is_component_v<T> )
inline auto make_component(Args &&... args) noexcept (false) {
  auto component = std::shared_ptr<T> {new T(std::forward<Args>(args)...)};
  component->init();
  return component;
}

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

#include <tui++/Screen.h>
