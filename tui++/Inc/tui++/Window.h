#pragma once

#include <tui++/RootPane.h>
#include <tui++/Shadow.h>
#include <tui++/WindowMouseEventDispatcher.h>

#include <tui++/event/WindowEvent.h>

#include <optional>
#include <string_view>

namespace tui {

enum class WindowType {
  NORMAL,
  POPUP,
  UTILITY
};

class Screen;
class KeyboardFocusManager;

class Window: public ComponentExtension<Component, WindowEvent>, public RootPaneContainer {
  using base = Component;

  std::shared_ptr<Window> owner;
  const WindowType type;

  std::vector<std::weak_ptr<Window>> owned_windows;
  std::shared_ptr<WindowMouseEventDispatcher> mouse_event_dispatcher;

  Property<std::string> title { this, "Title" };
  Property<bool> focusable_window_state { this, "FocusableWindowState", true };
  Property<bool> always_on_top { this, "AlwaysOnTop", false };

  // The window's drop shadow, installed from the theme by its kind (see
  // get_shadow_key) and overridable per window (set_shadow).
  Property<std::optional<Shadow>> shadow { this, "Shadow" };

  std::weak_ptr<Component> temporary_lost_component;

  struct {
    unsigned in_show :1 = false;
    unsigned before_first_show :1 = true;
    unsigned opened :1 = false;
  };

protected:
  std::shared_ptr<RootPane> root_pane;

protected:
  Window(WindowType type = WindowType::NORMAL) :
      type(type) {
  }

  Window(const std::shared_ptr<Window> &owner, WindowType type = WindowType::NORMAL) :
      owner(owner), type(type) {
  }

  virtual ~Window() {
    if (this->owner) {
      // Drop this window's weak entry in the owner's list. shared_from_this()
      // is unusable here: when the last reference is being released the weak
      // handle has already expired (and the entry expires on its own anyway).
      with_tree_locked([this] {
        std::erase_if(this->owner->owned_windows, [this](auto const &candidate) {
          return candidate.expired() or candidate.lock().get() == this;
        });
      });
    }
  }

  virtual void init() override;
  virtual std::shared_ptr<RootPane> create_root_pane() const;

  friend class Screen;

protected:
  void add_impl(const std::shared_ptr<Component> &c, const Constraints &constraints, int z_order) noexcept (false) override;
  void add_notify() override;

  void paint_children(Graphics &g) override;

  void show() override;
  void hide() override;

  void dispatch_event_to_self(Event &e) {
    base::dispatch_event(e);
  }

  // The theme key of the shadow a window of this kind casts. A popup window's
  // key is chosen by whatever opened it (see Popup), so a combo box dropdown
  // and a menu popup can shade differently; a frame fills the screen and
  // defines no shadow of its own.
  virtual std::string_view get_shadow_key() const {
    return "Window.Shadow";
  }

  // Shades the window's shadow into the graphics before the window's content
  // is painted over its middle (see Screen::paint). The graphics is the
  // screen's own, in screen coordinates and untranslated, so the shadow lands
  // where the window is -- not where the window's own paint context sits.
  void paint_shadow(Graphics &g);

public:
  const std::string& get_title() const {
    return this->title;
  }

  void set_title(const std::string &title) {
    this->title = title;
  }

  // "tui::Frame(MenuBar demo)": windows identify themselves by their title
  // (an explicit name still wins).
  std::string to_string() const override;

  virtual void remove(const std::shared_ptr<Component> &c) override;
  virtual void set_layout(const std::shared_ptr<Layout> &layout) override;
  virtual void dispatch_event(Event &e) override;

  // The shadow this window casts, if any: the theme's shadow of its kind,
  // unless the application set one, and nothing at all while the global
  // shadow switch is off (see Shadow::set_enabled).
  std::optional<Shadow> get_shadow() const;

  void set_shadow(std::optional<Shadow> const &shadow);

  virtual bool is_opaque() const override;

  virtual bool is_displayable() const override {
    return true;
  }

  virtual void paint(Graphics &g) override {
    validate();
    base::paint(g);
  }

  bool is_focus_cycle_root() const override {
    return true;
  }

  WindowType get_type() const {
    return this->type;
  }

  std::shared_ptr<Window> get_owner() const {
    return this->owner;
  }

  bool is_focused() const {
    return KeyboardFocusManager::single->get_focused_window().get() == this;
  }

  std::shared_ptr<Component> get_focus_owner() const;

  std::shared_ptr<Component> get_most_recent_focus_owner() const;

  bool is_focusable_window() const;

  bool get_focusable_window_state() const {
    return this->focusable_window_state;
  }
  void set_focusable_window_state(bool state);

  void set_always_on_top(bool value);
  bool is_always_on_top() const {
    return this->always_on_top;
  }

  std::shared_ptr<RootPane> get_root_pane() const override final;
  std::shared_ptr<Component> get_content_pane() const override final;
  void set_content_pane(const std::shared_ptr<Component> &content_pane) override final;
  std::shared_ptr<LayeredPane> get_layered_pane() const override final;
  void set_layered_pane(const std::shared_ptr<LayeredPane> &layered_pane) override final;
  std::shared_ptr<Component> get_glass_pane() const override final;
  void set_glass_pane(const std::shared_ptr<Component> &glass_pane) override final;

  void set_root_pane(const std::shared_ptr<RootPane> &root_pane);

  void pack();

private:
  std::shared_ptr<Component> get_temporary_lost_component() const {
    return this->temporary_lost_component.lock();
  }

  std::shared_ptr<Component> set_temporary_lost_component(const std::shared_ptr<Component> &component) {
    auto prev = get_temporary_lost_component();
    // Check that "component" is an acceptable focus owner and don't store it otherwise
    // - or later we will have problems with opposite while handling  WINDOW_GAINED_FOCUS
    if (not component or component->can_be_focus_owner()) {
      this->temporary_lost_component = component;
    } else {
      this->temporary_lost_component.reset();
    }
    return prev;
  }

  void to_front();

  // The screen rectangle the window's shadow covers (empty when it casts
  // none).
  Rectangle get_shadow_area() const;

  void enable_events_for_dispatching(EventTypeMask event_mask) {
    if (this->mouse_event_dispatcher) {
      // The dispatcher tracks the pointer to synthesize MOUSE_ENTERED /
      // MOUSE_EXITED events for the components that listen for them. No
      // component enables plain MOUSE_MOVED, so without this the over events
      // would never fire and hover (rollover) would be dead.
      if (event_mask & EventType::MOUSE_OVER) {
        event_mask |= EventType::MOUSE_MOVE;
      }
      this->mouse_event_dispatcher->enable_events(event_mask);
    }
  }

  void add_owned_window(const std::shared_ptr<Window> &w);
  void remove_owned_window(const std::shared_ptr<Window> &w);

  friend class Component;
  friend class KeyboardFocusManager;
  friend class WindowMouseEventDispatcher;
};

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source_window, Type type, const std::shared_ptr<Window> &opposite_window) :
    Event(source_window, type), opposite_window(opposite_window) {
}

constexpr WindowEvent::WindowEvent(const std::shared_ptr<Window> &source, Type type) :
    WindowEvent(source, type, nullptr) {
}

constexpr std::shared_ptr<Window> WindowEvent::get_window() const {
  return std::dynamic_pointer_cast<Window>(this->source);
}

}
