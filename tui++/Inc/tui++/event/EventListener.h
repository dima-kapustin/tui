#pragma once

#include <functional>
#include <type_traits>

namespace tui {

template<typename Event>
class EventListener;

template<typename Event>
using FunctionalEventListener = std::function<void(Event &e)>;

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
class EventListener<MouseEvent> : public BasicEventListener<MouseEvent> {
public:
  virtual void mouse_pressed(MouseEvent &e) = 0;
  virtual void mouse_released(MouseEvent &e) = 0;
};

template<>
class EventListener<MouseClickEvent> : public BasicEventListener<MouseClickEvent> {
public:
  virtual void mouse_clicked(MouseClickEvent &e) = 0;
};

template<>
class EventListener<MouseHoverEvent> : public BasicEventListener<MouseHoverEvent> {
public:
  virtual void mouse_entered(MouseHoverEvent &e) = 0;
  virtual void mouse_exited(MouseHoverEvent &e) = 0;
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

}
