#pragma once

#include <functional>
#include <type_traits>

namespace tui {

template<typename Event>
class EventListener;

template<typename Event>
using FunctionalEventListener = std::function<void(Event &e)>;

class EventListenerBase {
public:
  virtual ~EventListenerBase() {
  }
};
//
//template<>
//class EventListener<ActionEvent>: public EventListenerBase {
//public:
//  virtual void action_performed(ActionEvent &e) = 0;
//};

template<typename Event>
class EventListener: public EventListenerBase {
public:
  virtual void on_event(Event &e) = 0;
};

}
