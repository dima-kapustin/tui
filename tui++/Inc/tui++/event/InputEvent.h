#pragma once

#include <tui++/event/BasicEvent.h>
#include <tui++/util/EnumFlags.h>

#include <chrono>

namespace tui {

using EventClock = std::chrono::utc_clock;

class InputEvent: public BasicEvent {
public:
  enum Modifier {
    NO_MODIFIERS = 0,
    /** The shift key modifier constant */
    SHIFT_DOWN = 1 << 0,
    /** The control key modifier constant */
    CTRL_DOWN = 1 << 1,
    /** The alt key modifier constant */
    ALT_DOWN = 1 << 2,
    META_DOWN = 1 << 4,
    LEFT_BUTTON_DOWN = 1 << 5,
    MIDDLE_BUTTON_DOWN = 1 << 6,
    RIGHT_BUTTON_DOWN = 1 << 7,
  };

  using Modifiers = util::EnumFlags<Modifier>;

protected:
  constexpr InputEvent() = default;
  constexpr InputEvent(const std::shared_ptr<Component> &source, Modifiers modifiers, const EventClock::time_point &when = EventClock::now()) :
      BasicEvent(source), modifiers(modifiers), when(when) {
  }

public:
  Modifiers modifiers;
  EventClock::time_point when = EventClock::now();
};

}
