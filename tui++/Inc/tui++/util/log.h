#pragma once

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

namespace tui::util {

extern std::ostream *event_log;
extern std::ostream *focus_log;
extern std::ostream *platform_log;

// Milliseconds elapsed since the first log timestamp was taken (steady clock,
// so it is monotonic and unaffected by wall-clock changes). Shared across
// translation units because it lives in an inline function.
inline double log_now_ms() {
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// The "[<ms> ms] " prefix carried by every log line: fixed width (so lines
// line up) and always exactly one decimal, whatever the magnitude. The number
// is formatted by hand so the decimal point stays '.' even when the process
// locale would make %f print a comma.
inline std::string log_now_prefix() {
  auto ms = log_now_ms();
  auto whole = static_cast<long long>(ms);
  auto tenths = static_cast<int>((ms - static_cast<double>(whole)) * 10.0 + 0.5);
  if (tenths >= 10) {
    tenths = 0;
    ++whole;
  }
  char buffer[48];
  std::snprintf(buffer, sizeof buffer, "[%7lld.%d ms] ", whole, tenths);
  return buffer;
}

}

// Each log macro expands to the timestamp and the message, guarded by a null
// stream check so there is no cost when the stream is not enabled. The macros
// are also #ifndef-guarded: a build can pre-define any of them to an empty
// macro (for example ((void)0)) to compile the corresponding logging out
// entirely.

#ifndef log_timestamp
#define log_timestamp() tui::util::log_now_prefix()
#endif

#ifndef log_event_ln
#define log_event_ln(x) if (tui::util::event_log) *tui::util::event_log << log_timestamp() << "event: " << x << std::endl
#endif

#ifndef log_repaint_ln
#define log_repaint_ln(x) if (tui::util::event_log) *tui::util::event_log << log_timestamp() << "repaint: " << x << std::endl
#endif

#ifndef log_dispatch_ln
#define log_dispatch_ln(x) if (tui::util::event_log) *tui::util::event_log << log_timestamp() << "dispatch: " << x << std::endl
#endif

#ifndef log_focus_if_ln
#define log_focus_if_ln(condition, x) if (tui::util::focus_log and condition) *tui::util::focus_log << log_timestamp() << "focus: " << x << std::endl
#endif

#ifndef log_focus_ln
#define log_focus_ln(x) log_focus_if_ln(true, x)
#endif

#ifndef log_platform_ln
#define log_platform_ln(x) if (tui::util::platform_log) *tui::util::platform_log << log_timestamp() << "platform: " << x << std::endl
#endif
