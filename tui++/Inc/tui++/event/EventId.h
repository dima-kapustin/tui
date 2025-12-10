#pragma once

#include <tui++/util/EnumFlags.h>

#include <utility>

namespace tui {

enum class EventType : unsigned {
  KEY = 1 << 0,
  ITEM = KEY << 1,
  FOCUS = ITEM << 1,
  MOUSE = FOCUS << 1,
  MOUSE_OVER = MOUSE << 1,
  MOUSE_MOVE = MOUSE_OVER << 1,
  MOUSE_DRAG = MOUSE_MOVE << 1,
  MOUSE_CLICK = MOUSE_DRAG << 1,
  MOUSE_WHEEL = MOUSE_CLICK << 1,
  ACTION = MOUSE_WHEEL << 1,
  WINDOW = ACTION << 1,
  COMPONENT = WINDOW << 1,
  CONTAINER = COMPONENT << 1,
  HIERARCHY = CONTAINER << 1,
  HIERARCHY_BOUNDS = HIERARCHY << 1,
  INVOCATION = HIERARCHY_BOUNDS << 1,
};

using EventTypeMask = util::EnumFlags<EventType>;

class EventId {
  constexpr static auto SUB_TYPE_BIT_LEN = 8;

public:
  union {
    unsigned id;
    struct {
      unsigned sub_type :SUB_TYPE_BIT_LEN;
      EventType type :24;
    };
  };

public:
  constexpr EventId(EventType type, unsigned sub_type = 0) :
      id((std::to_underlying(type) << SUB_TYPE_BIT_LEN) | sub_type) {
  }

  template<typename Id>
  explicit constexpr EventId(Id id) :
      id(std::to_underlying(id)) {
  }

  constexpr operator unsigned() const {
    return this->id;
  }

  friend constexpr EventTypeMask operator&(const EventId &id, const EventTypeMask &mask) {
    return mask & EventType(id.type);
  }

  friend constexpr EventTypeMask operator&(const EventTypeMask &mask, const EventId &id) {
    return mask & EventType(id.type);
  }
};

static_assert(sizeof(EventId) <= sizeof(unsigned));

template<EventType type, unsigned sub_type = 0>
constexpr EventId event_id_v = { type, sub_type };

}

