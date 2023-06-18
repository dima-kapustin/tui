#pragma once

#include <iostream>

namespace tui::util {

extern std::ostream *log;

}

#define log_event_ln(x) {if(tui::util::log) { *tui::util::log << "event: " << x << std::endl; }}
#define log_focus_ln(x) {if(tui::util::log) { *tui::util::log << "focus: " << x << std::endl; }}
