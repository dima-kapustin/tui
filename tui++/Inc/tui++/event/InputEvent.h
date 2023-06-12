#pragma once

#include <tui++/event/BasicEvent.h>
#include <tui++/util/EnumFlags.h>

#include <chrono>

namespace tui {

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

    META_DOWN = 1 << 4
  };

  using Modifiers = util::EnumFlags<Modifier>;

protected:
  constexpr InputEvent() = default;
  constexpr InputEvent(const std::shared_ptr<Component> &source, Modifiers modifiers) :
      BasicEvent(source), modifiers(modifiers) {
  }

public:
  Modifiers modifiers;
  std::chrono::utc_clock::time_point when = std::chrono::utc_clock::now();
};
}
