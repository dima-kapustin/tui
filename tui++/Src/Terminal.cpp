#include <iostream>

#include <tui++/Screen.h>
#include <tui++/Terminal.h>
#include <tui++/Component.h>

namespace tui {

std::chrono::milliseconds Terminal::read_event_timeout { 20 };

Terminal::InputParser Terminal::input_parser;
std::vector<Terminal::Option> Terminal::set_options;

MouseEvent Terminal::prev_mouse_event { };

Terminal::Clock::time_point Terminal::prev_mouse_press_time;
Terminal::Clock::time_point Terminal::prev_mouse_click_time;

std::chrono::milliseconds Terminal::mouse_click_detection_timeout { 400 };
std::chrono::milliseconds Terminal::mouse_double_click_detection_timeout { 500 };

void Terminal::set_option(Option option) {
  struct SetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?" << int(option) << 'h';
    }
    void operator()(const ModifyKeyboardOption &option) {
      std::cout << "\x1b[>0;" << int(option) << 'm';
    }
    void operator()(const ModifyCursorKeysOption &option) {
      std::cout << "\x1b[>1;" << int(option) << 'm';
    }
    void operator()(const ModifyFunctionKeysOption &option) {
      std::cout << "\x1b[>2;" << int(option) << 'm';
    }
    void operator()(const ModifyOtherKeysOption &option) {
      std::cout << "\x1b[>4;" << int(option) << 'm';
    }
  };

  std::visit(SetOption(), option);
  set_options.emplace_back(option);
}

void Terminal::reset_option(Option option) {
  struct ResetOption {
    void operator()(const DECModeOption &option) {
      std::cout << "\x1b[?" << int(option) << 'l';
    }
    void operator()(const ModifyKeyboardOption&) {
      std::cout << "\x1b[>0m";
    }
    void operator()(const ModifyCursorKeysOption&) {
      std::cout << "\x1b[>1m";
    }
    void operator()(const ModifyFunctionKeysOption&) {
      std::cout << "\x1b[>2m";
    }
    void operator()(const ModifyOtherKeysOption&) {
      std::cout << "\x1b[>4m";
    }
  };

  std::visit(ResetOption(), option);

  set_options.erase( //
      std::remove(set_options.begin(), set_options.end(), option), //
      set_options.end());
}

void Terminal::init() {
  reset_option(DECModeOption::LINE_WRAP);

  set_option(DECModeOption::MOUSE_VT200);
  set_option(DECModeOption::MOUSE_ANY_EVENT);
  set_option(DECModeOption::MOUSE_URXVT_EXT_MODE);
  set_option(DECModeOption::MOUSE_SGR_EXT_MODE);

  flush();
}

void Terminal::deinit() {
  reset_option(DECModeOption::MOUSE_SGR_EXT_MODE);
  reset_option(DECModeOption::MOUSE_URXVT_EXT_MODE);
  reset_option(DECModeOption::MOUSE_ANY_EVENT);
  reset_option(DECModeOption::MOUSE_VT200);

  flush();
}

void Terminal::flush() {
  std::cout << std::flush;
}

void Terminal::new_resize_event() {
  std::cout << "Resize Event" << std::endl;
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
  Screen::post(std::make_unique<Event>(nullptr, key_code, modifiers));
}

void Terminal::new_mouse_event(MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y) {
  auto wheel_rotation = 0;
  auto adjusted_type = type;
  if (type != MouseEvent::MOUSE_WHEEL) {
    if ((prev_mouse_event.type == type and prev_mouse_event.button == button) or button == MouseEvent::NO_BUTTON) {
      if (prev_mouse_event.x == x and prev_mouse_event.y == y) {
        return;
      } else {
        if (type == MouseEvent::MOUSE_PRESSED and button != MouseEvent::NO_BUTTON) {
          adjusted_type = MouseEvent::MOUSE_DRAGGED;
        } else {
          adjusted_type = MouseEvent::MOUSE_MOVED;
        }
      }
    }
  } else {
    wheel_rotation = button == 0 ? -1 : 1;
  }

  auto component = Screen::get_component_at(x, y);
  auto p = convert_point_from_screen( { x, y }, component);

  Screen::post(std::make_unique<Event>(component, adjusted_type, button, modifiers, p.x, p.y, wheel_rotation, false));

  if (adjusted_type == MouseEvent::MOUSE_PRESSED) {
    prev_mouse_press_time = Clock::now();
  } else if (adjusted_type == MouseEvent::MOUSE_RELEASED) {
    if (prev_mouse_event.button == button and (Clock::now() - prev_mouse_press_time) < mouse_click_detection_timeout) {
      if ((Clock::now() - prev_mouse_click_time) < mouse_double_click_detection_timeout) {
        Screen::post(std::make_unique<Event>(component, MouseEvent::MOUSE_CLICKED, button, modifiers, p.x, p.y, 2, false));
        prev_mouse_click_time = { };
      } else {
        Screen::post(std::make_unique<Event>(component, MouseEvent::MOUSE_CLICKED, button, modifiers, p.x, p.y, 1, false));
        prev_mouse_click_time = Clock::now();
      }
    }
  }

  prev_mouse_event.type = type;
  prev_mouse_event.button = button;
  prev_mouse_event.modifiers = modifiers;
  prev_mouse_event.x = x;
  prev_mouse_event.y = y;
}
}
