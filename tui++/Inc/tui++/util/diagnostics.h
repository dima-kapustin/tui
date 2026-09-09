#pragma once

// Process-wide crash diagnostics: global exception and signal handlers that
// log why the process is dying before the default handling (console restore,
// exit status) takes over. An uncaught C++ exception or a fatal signal that
// used to end the process with a bare CRT message ("terminate called after
// throwing an instance of 'std::bad_weak_ptr'") now prints:
//
//   [tui++ fatal] uncaught exception: std::terminate called
//     what(): bad_weak_ptr
//     backtrace:
//       #0 0x00007FF6...
//       ...
//
// plus, when the exception escaped an event dispatch, a context block naming
// the event, its source component and the focus owner (see Screen.cpp).
// Backtrace frames are printed as raw addresses (symbolizing them needs the
// binary: `addr2line -e <exe> -f -C 0x...`); nothing in the logging path
// allocates, so the same writer is safe inside a signal handler.
//
// The terminals install these handlers with their console-restore routine
// (see install_crash_handlers), so a crash still puts the terminal back, and
// their own fatal-signal handlers call log_fatal_signal before restoring.

namespace tui::util {

// Installs the process-wide terminate handler (uncaught exceptions) and, on
// Windows, the unhandled-exception filter that hardware faults (access
// violations, ...) go through. `restore_fatal_state` is called (when set)
// right before the process dies, after the diagnostics were written; the
// terminal backends pass their async-signal-safe console restore here.
// Idempotent.
void install_crash_handlers(void (*restore_fatal_state)() = nullptr);

// Logs a fatal signal to stderr: name, number and a backtrace. Meant to be
// called from the platform's own fatal-signal handler (the terminals restore
// the console and re-raise afterwards); non-fatal signals (SIGINT, SIGTERM,
// ...) pass silently, and once a crash was reported -- by this function, the
// terminate handler or the exception filter -- later calls log nothing (the
// terminate handler aborts through SIGABRT, which would otherwise double the
// report). Async-signal-safe in practice.
void log_fatal_signal(int sig);

}
