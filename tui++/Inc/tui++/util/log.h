#pragma once

#include <iostream>

namespace tui::util {

extern std::ostream *event_log;
extern std::ostream *focus_log;

}

#define log_event_ln(x) {if(tui::util::event_log) { *tui::util::event_log << "event: " << x << std::endl; }}
#define log_focus_ln(x) {if(tui::util::focus_log) { *tui::util::focus_log << "focus: " << x << std::endl; }}
