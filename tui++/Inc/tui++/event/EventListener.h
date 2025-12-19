#pragma once

#include <functional>
#include <type_traits>

namespace tui {

template<typename Event>
class EventListener;

template<typename Event>
using FunctionalEventListener = std::function<void(Event &e)>;

class ActionEvent;
class ComponentEvent;
class ContainerEvent;
class FocusEvent;
class ItemEvent;
class KeyEvent;
class MouseDragEvent;
class MouseMoveEvent;
class MouseOverEvent;
class MouseClickEvent;
class MousePressEvent;
class MouseWheelEvent;
class WindowEvent;

template<typename MouseEvent>
requires (std::is_base_of_v<MousePressEvent, MouseEvent> or std::is_base_of_v<MouseDragEvent, MouseEvent> or std::is_base_of_v<MouseOverEvent, MouseEvent>)
class MenuDragMouseEvent;

template<typename Event>
class BasicEventListener {
public:
  using event_type = Event;

public:
  virtual ~BasicEventListener() {
  }

protected:
  BasicEventListener() = default;
};

template<>
class EventListener<ActionEvent> : public BasicEventListener<ActionEvent> {
public:
  virtual void action_performed(ActionEvent &e) = 0;
};

template<>
class EventListener<ItemEvent> : public BasicEventListener<ItemEvent> {
public:
  virtual void item_state_changed(ItemEvent &e) = 0;
};

template<>
class EventListener<KeyEvent> : public BasicEventListener<KeyEvent> {
public:
  virtual void key_pressed(KeyEvent &e) = 0;
  virtual void key_typed(KeyEvent &e) = 0;
};

template<>
class EventListener<MousePressEvent> : public BasicEventListener<MousePressEvent> {
public:
  virtual void mouse_pressed(MousePressEvent &e) = 0;
  virtual void mouse_released(MousePressEvent &e) = 0;
};

template<>
class EventListener<MouseClickEvent> : public BasicEventListener<MouseClickEvent> {
public:
  virtual void mouse_clicked(MouseClickEvent &e) = 0;
};

template<>
class EventListener<MouseOverEvent> : public BasicEventListener<MouseOverEvent> {
public:
  virtual void mouse_entered(MouseOverEvent &e) = 0;
  virtual void mouse_exited(MouseOverEvent &e) = 0;
};

template<>
class EventListener<MouseMoveEvent> : public BasicEventListener<MouseMoveEvent> {
public:
  virtual void mouse_moved(MouseMoveEvent &e) = 0;
};

template<>
class EventListener<MouseDragEvent> : public BasicEventListener<MouseDragEvent> {
public:
  virtual void mouse_dragged(MouseDragEvent &e) = 0;
};

template<>
class EventListener<MouseWheelEvent> : public BasicEventListener<MouseWheelEvent> {
public:
  virtual void mouse_wheel_moved(MouseWheelEvent &e) = 0;
};

template<>
class EventListener<FocusEvent> : public BasicEventListener<FocusEvent> {
public:
  virtual void focus_gained(FocusEvent &e) = 0;
  virtual void focus_lost(FocusEvent &e) = 0;
};

template<>
class EventListener<WindowEvent> : public BasicEventListener<WindowEvent> {
public:
  virtual void window_opened(WindowEvent &e) = 0;
  virtual void window_closing(WindowEvent &e) = 0;
  virtual void window_closed(WindowEvent &e) = 0;
  virtual void window_activated(WindowEvent &e) = 0;
  virtual void window_deactivated(WindowEvent &e) = 0;
  virtual void window_gained_focus(WindowEvent &e) = 0;
  virtual void window_lost_focus(WindowEvent &e) = 0;
  virtual void window_iconified(WindowEvent &e) = 0;
  virtual void window_deiconified(WindowEvent &e) = 0;
  virtual void window_state_changed(WindowEvent &e) = 0;
};


template<typename MouseEvent>
requires (std::is_base_of_v<MousePressEvent, MouseEvent> or std::is_base_of_v<MouseDragEvent, MouseEvent> or std::is_base_of_v<MouseOverEvent, MouseEvent>)
class MenuDragMouseListener;

template<>
class MenuDragMouseListener<MouseOverEvent> : public BasicEventListener<MenuDragMouseEvent<MouseOverEvent>> {
public:
  virtual void menu_drag_mouse_entered(MenuDragMouseEvent<MouseOverEvent> &e) = 0;
  virtual void menu_drag_mouse_exited(MenuDragMouseEvent<MouseOverEvent> &e) = 0;
};

template<>
class MenuDragMouseListener<MouseDragEvent> : public BasicEventListener<MenuDragMouseEvent<MouseDragEvent>> {
public:
  virtual void menu_drag_mouse_dragged(MenuDragMouseEvent<MouseDragEvent> &e) = 0;
};

template<>
class MenuDragMouseListener<MousePressEvent> : public BasicEventListener<MenuDragMouseEvent<MousePressEvent>> {
public:
  virtual void menu_drag_mouse_released(MenuDragMouseEvent<MousePressEvent> &e) = 0;
};

}
