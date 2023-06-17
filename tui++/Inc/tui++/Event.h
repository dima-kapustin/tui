#pragma once

#include <tui++/event/KeyEvent.h>
#include <tui++/event/ItemEvent.h>
#include <tui++/event/FocusEvent.h>
#include <tui++/event/MouseEvent.h>
#include <tui++/event/ActionEvent.h>
#include <tui++/event/WindowEvent.h>
#include <tui++/event/InvocationEvent.h>

#include <variant>

namespace tui {

using Event = std::variant<
/**/std::monostate,
/**/KeyEvent,
/**/ItemEvent,
/**/FocusEvent,
/**/MouseEvent,
/**/MouseMoveEvent,
/**/MouseDragEvent,
/**/MouseClickEvent,
/**/MouseWheelEvent,
/**/ActionEvent,
/**/WindowEvent,
/**/InvocationEvent>;

std::string to_string(KeyEvent::KeyCode key_code);

std::ostream& operator<<(std::ostream &os, const Event &event);

}
