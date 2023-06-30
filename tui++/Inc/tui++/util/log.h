#pragma once

#include <iostream>

namespace tui::util {

extern std::ostream *event_log;
extern std::ostream *focus_log;
extern std::ostream *platform_log;

}

#define log_event_ln(x) {if(tui::util::event_log) { *tui::util::event_log << "event: " << x << std::endl; }}
#define log_focus_ln(x) {if(tui::util::focus_log) { *tui::util::focus_log << "focus: " << x << std::endl; }}
#define log_platform_ln(x) {if(tui::util::platform_log) { *tui::util::platform_log << "platform: " << x << std::endl; }}
