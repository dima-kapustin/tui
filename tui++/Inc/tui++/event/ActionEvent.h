#pragma once

#include <tui++/event/InputEvent.h>
#include <tui++/util/string.h>

namespace tui {

using ActionKey = u8string;

struct ActionEvent: public BasicEvent {
  constexpr static auto NO_MODIFIERS = InputEvent::NO_MODIFIERS;
  constexpr static auto SHIFT_DOWN = InputEvent::SHIFT_DOWN;
  constexpr static auto CTRL_DOWN = InputEvent::CTRL_DOWN;
  constexpr static auto ALT_DOWN = InputEvent::ALT_DOWN;
  constexpr static auto META_DOWN = InputEvent::META_DOWN;

  using Modifiers = InputEvent::Modifiers;

public:
  ActionEvent(const std::shared_ptr<Component> &source, const ActionKey &action_command, const Modifiers &modifiers) :
      BasicEvent(source), action_command(action_command), modifiers(modifiers) {
  }

public:
  ActionKey action_command;
  Modifiers modifiers;
  std::chrono::utc_clock::time_point when = std::chrono::utc_clock::now();
};

}
