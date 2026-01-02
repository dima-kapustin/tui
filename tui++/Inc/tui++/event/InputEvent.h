#pragma once

#include <tui++/event/ComponentEvent.h>

namespace tui {

class InputEvent: public ComponentEvent {
public:
  enum Modifier {
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
  constexpr static auto NO_MODIFIERS = Modifiers { };

protected:
  constexpr InputEvent() = default;
  template<typename Id>
  constexpr InputEvent(const std::shared_ptr<Component> &source, const Id &id, Modifiers modifiers, const EventClock::time_point &when = EventClock::now()) :
      ComponentEvent(source, id, when), modifiers(modifiers) {
  }

  InputEvent(InputEvent const&) = default;

public:
  Modifiers modifiers;
};

}
