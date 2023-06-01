#include <tui++/Terminal.h>

#include <iostream>

namespace tui {

bool Terminal::quit = false;
EventQueue Terminal::event_queue;
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

void Terminal::run_event_loop() {
  while (not quit) {
    read_events();
    if (auto event = event_queue.pop(INPUT_TIMEOUT)) {
      switch (event->type) {
      case Event::KEY:
        std::cout << "Key Pressed: " << to_string(event->key.key_code) << std::endl;
        break;

      case Event::MOUSE:
        std::cout << "Mouse ";

        if (event->mouse.type == MouseEvent::MOUSE_MOVED) {
          std::cout << "Moved with ";
        } else if (event->mouse.type == MouseEvent::MOUSE_DRAGGED) {
          std::cout << "Dragged with ";
        }

        if (event->mouse.type != MouseEvent::MOUSE_WHEEL) {
          switch (event->mouse.button) {
          case MouseEvent::LEFT_BUTTON:
            std::cout << "Left Button";
            break;
          case MouseEvent::MIDDLE_BUTTON:
            std::cout << "Middle Button";
            break;
          case MouseEvent::RIGHT_BUTTON:
            std::cout << "Right Button";
            break;
          case MouseEvent::NO_BUTTON:
            std::cout << "No Button";
            break;
          }
        }

        switch (event->mouse.type) {
        case MouseEvent::MOUSE_PRESSED:
          std::cout << " Pressed";
          break;
        case MouseEvent::MOUSE_RELEASED:
          std::cout << " Released";
          break;
        case MouseEvent::MOUSE_CLICKED:
          if (event->mouse.click_count == 2) {
            std::cout << " Double Clicked";
          } else {
            std::cout << " Clicked";
          }
          break;

        case MouseEvent::MOUSE_WHEEL:
          std::cout << " Wheel scrolled by " << event->mouse.weel_rotation;
          break;

        default:
          break;
        }

        if (event->mouse.modifiers) {
          std::cout << " with ";
          if (event->mouse.modifiers & InputEvent::ALT_MASK) {
            std::cout << "Alt";
          }
          if (event->mouse.modifiers & InputEvent::SHIFT_MASK) {
            std::cout << "Shift";
          }
          if (event->mouse.modifiers & InputEvent::CTRL_MASK) {
            std::cout << "Ctrl";
          }
          if (event->mouse.modifiers & InputEvent::META_MASK) {
            std::cout << "Meta";
          }
        }

        std::cout << " at " << event->mouse.x << "," << event->mouse.y << std::endl;
        break;

      case Event::INVOCATION:
        std::cout << "Invocation Event" << std::endl;
        break;

      case Event::UNDEFINED:
        std::cout << "UNDEFINED Event" << std::endl;
        break;
      }

    }
  }
}

void Terminal::flush() {
  std::cout << std::flush;
}

void Terminal::new_resize_event() {
  std::cout << "Resize Event" << std::endl;
}

void Terminal::new_key_event(KeyEvent::KeyCode key_code, InputEvent::Modifiers modifiers) {
  Terminal::event_queue.push(std::make_unique<Event>(key_code, modifiers));
}

void Terminal::new_mouse_event(MouseEvent::Type type, MouseEvent::Button button, InputEvent::Modifiers modifiers, int x, int y) {
  auto adjusted_type = type;
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

  int click_count = type == MouseEvent::MOUSE_WHEEL ? (button == 0 ? -1 : 1) : 0;
  Terminal::event_queue.push(std::make_unique<Event>(adjusted_type, button, modifiers, x, y, click_count, false));

  if (adjusted_type == MouseEvent::MOUSE_PRESSED) {
    prev_mouse_press_time = Clock::now();
  } else if (adjusted_type == MouseEvent::MOUSE_RELEASED) {
    if (prev_mouse_event.button == button and (Clock::now() - prev_mouse_press_time) < mouse_click_detection_timeout) {
      if ((Clock::now() - prev_mouse_click_time) < mouse_double_click_detection_timeout) {
        Terminal::event_queue.push(std::make_unique<Event>(MouseEvent::MOUSE_CLICKED, button, modifiers, x, y, 2, false));
        prev_mouse_click_time = { };
      } else {
        Terminal::event_queue.push(std::make_unique<Event>(MouseEvent::MOUSE_CLICKED, button, modifiers, x, y, 1, false));
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
