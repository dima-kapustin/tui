#pragma once

#include <tui++/event/InputEvent.h>
#include <tui++/util/string.h>

#include <functional>

namespace tui {

using ActionKey = u8string;

struct ActionEvent: public Event {
  constexpr static auto NO_MODIFIERS = InputEvent::NO_MODIFIERS;
  constexpr static auto SHIFT_DOWN = InputEvent::SHIFT_DOWN;
  constexpr static auto CTRL_DOWN = InputEvent::CTRL_DOWN;
  constexpr static auto ALT_DOWN = InputEvent::ALT_DOWN;
  constexpr static auto META_DOWN = InputEvent::META_DOWN;
  constexpr static auto LEFT_BUTTON_DOWN = InputEvent::LEFT_BUTTON_DOWN;
  constexpr static auto MIDDLE_BUTTON_DOWN = InputEvent::LEFT_BUTTON_DOWN;
  constexpr static auto RIGHT_BUTTON_DOWN = InputEvent::LEFT_BUTTON_DOWN;

  using Modifiers = InputEvent::Modifiers;

public:
  ActionEvent(const std::shared_ptr<Object> &source, const ActionKey &action_command, const Modifiers &modifiers, EventClock::time_point when = std::chrono::utc_clock::now()) :
      Event(source, EventType::ACTION), action_command(action_command), modifiers(modifiers), when(when) {
  }

public:
  ActionKey action_command;
  Modifiers modifiers;
  EventClock::time_point when;
};

using ActionEventListener = std::function<void(ActionEvent &e)>;

}
