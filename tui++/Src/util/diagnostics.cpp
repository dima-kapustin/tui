#include <tui++/util/diagnostics.h>

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>

#if defined(_WIN32)
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601 // RtlCaptureStackBackTrace needs 0x0600
# endif
# include <windows.h>
#else
# include <unistd.h>
# if __has_include(<execinfo.h>)
#  include <execinfo.h>
# endif
# if defined(__GNUC__) and not __has_include(<execinfo.h>)
#  include <unwind.h>
# endif
#endif

namespace tui::util {

namespace {

constexpr int MAX_FRAMES = 32;

// The signals that mean the process crashed (as opposed to a deliberate
// termination like Ctrl+C). SIGBUS exists on POSIX only.
bool is_crash_signal(int sig) {
  switch (sig) {
  case SIGABRT:
  case SIGSEGV:
  case SIGFPE:
  case SIGILL:
    return true;
#if !defined(_WIN32)
  case SIGBUS:
    return true;
#endif
  default:
    return false;
  }
}

const char *signal_name(int sig) {
  switch (sig) {
  case SIGABRT: return "SIGABRT";
  case SIGSEGV: return "SIGSEGV";
  case SIGFPE: return "SIGFPE";
  case SIGILL: return "SIGILL";
#if !defined(_WIN32)
  case SIGBUS: return "SIGBUS";
#endif
  default: return "?";
  }
}

// Captures up to `max` return addresses of the calling thread. Frame 0 is
// this function itself (dropped by the printers).
int capture_frames(void **frames, int max) {
#if defined(_WIN32)
  return int(::RtlCaptureStackBackTrace(0, ULONG(max), frames, nullptr));
#elif __has_include(<execinfo.h>)
  return ::backtrace(frames, max);
#else
  // libgcc unwinder fallback for POSIX targets without <execinfo.h>.
  struct Walk {
    void **frames;
    int max;
    int count = 0;
  } walk { frames, max };
  _Unwind_Backtrace([](struct _Unwind_Context *context, void *arg) -> _Unwind_Reason_Code {
    auto &walk = *static_cast<Walk*>(arg);
    if (walk.count < walk.max) {
      walk.frames[walk.count++] = reinterpret_cast<void*>(_Unwind_GetIP(context));
      return _URC_NO_REASON;
    }
    return _URC_END_OF_STACK;
  }, &walk);
  return walk.count;
#endif
}

// A fixed-buffer writer to stderr with no allocation and no stdio: the same
// code path serves the terminate handler, the Windows exception filter and
// the signal handlers. Long lines wrap at the buffer size (a crash log is
// best effort; nothing here may fail loudly).
class RawWriter {
  char buffer[2048];
  std::size_t used = 0;

  static void raw_write(char const *data, std::size_t size) {
#if defined(_WIN32)
    auto handle = ::GetStdHandle(STD_ERROR_HANDLE);
    if (not handle or handle == INVALID_HANDLE_VALUE) {
      return;
    }
    while (size > 0) {
      auto chunk = DWORD(size > 0x7FFF ? 0x7FFF : size);
      auto written = DWORD { };
      if (not ::WriteFile(handle, data, chunk, &written, nullptr) or written == 0) {
        break;
      }
      data += written;
      size -= std::size_t(written);
    }
#else
    while (size > 0) {
      auto n = ::write(STDERR_FILENO, data, size);
      if (n <= 0) {
        break;
      }
      data += n;
      size -= std::size_t(n);
    }
#endif
  }

public:
  void flush() {
    if (this->used > 0) {
      raw_write(this->buffer, this->used);
      this->used = 0;
    }
  }

  void append(char ch) {
    if (this->used == sizeof this->buffer) {
      this->flush();
    }
    this->buffer[this->used++] = ch;
  }

  void append(char const *text) {
    if (not text) {
      text = "(null)";
    }
    for (; *text; ++text) {
      this->append(*text);
    }
  }

  void append_dec(int value) {
    char digits[16];
    auto n = 0;
    if (value < 0) {
      this->append('-');
      value = -value;
    }
    do {
      digits[n++] = char('0' + value % 10);
      value /= 10;
    } while (value > 0);
    while (n > 0) {
      this->append(digits[--n]);
    }
  }

  // "0x" + the address, zero-padded to the pointer width (fixed-width frames
  // line up in the log and copy-paste cleanly).
  void append_address(std::uintptr_t address) {
    constexpr auto digits = sizeof(void*) * 2;
    char hex[3 + digits]; // 0x + digits + NUL
    hex[0] = '0';
    hex[1] = 'x';
    for (auto i = std::size_t { 0 }; i < digits; ++i) {
      auto nibble = (address >> ((digits - 1 - i) * 4)) & 0xF;
      hex[2 + i] = char(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
    }
    hex[2 + digits] = '\0';
    this->append(hex);
  }

  void append_backtrace(int skip = 1) {
    void *frames[MAX_FRAMES];
    auto count = capture_frames(frames, MAX_FRAMES);
    this->append("  backtrace:\n");
    for (auto i = skip; i < count; ++i) {
      this->append("    #");
      this->append_dec(i - skip);
      this->append(" ");
      this->append_address(std::uintptr_t(frames[i]));
      this->append("\n");
    }
  }
};

// The restore routine the terminal registered (console modes, termios, the
// alternate-screen escape sequence); called after the diagnostics were
// written, before the process dies.
void (*fatal_restore)() = nullptr;

// Set once a crash was reported: the terminate handler aborts through
// SIGABRT, whose platform handler would otherwise report the same crash a
// second time. sig_atomic_t so signal contexts can read it safely.
volatile std::sig_atomic_t crash_reported = 0;

void report_crash(RawWriter &writer, void (*restore)()) {
  crash_reported = 1;
  writer.flush();
  if (restore) {
    restore();
  }
}

[[noreturn]] void on_terminate() {
  RawWriter writer;
  writer.append("\n[tui++ fatal] uncaught exception: std::terminate called\n");
  if (auto exception = std::current_exception()) {
    try {
      std::rethrow_exception(exception);
    } catch (std::exception const &ex) {
      writer.append("  what(): ");
      writer.append(ex.what());
      writer.append("\n");
    } catch (...) {
      writer.append("  what(): <exception without a std::exception base>\n");
    }
  } else {
    // std::terminate was invoked directly: a noexcept function threw, a
    // destructor threw during unwinding, or the application called it.
    writer.append("  (no exception in flight: a noexcept function or destructor terminated)\n");
  }
  writer.append_backtrace();
  report_crash(writer, fatal_restore);
  std::abort(); // unreachable in practice: abort() raises SIGABRT, which the
  // platform handler re-raises with the default disposition
}

#if defined(_WIN32)
LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS *info) {
  RawWriter writer;
  if (not crash_reported) {
    writer.append("\n[tui++ fatal] unhandled exception 0x");
    writer.append_address(std::uintptr_t(info->ExceptionRecord->ExceptionCode));
    if (info->ExceptionRecord->ExceptionAddress) {
      writer.append(" at ");
      writer.append_address(std::uintptr_t(info->ExceptionRecord->ExceptionAddress));
    }
    writer.append("\n");
    writer.append_backtrace();
    report_crash(writer, fatal_restore);
  }
  // The exception is not recoverable; end the process with the exception
  // code as the exit status (0xC0000005 for an access violation, ...).
  ::TerminateProcess(::GetCurrentProcess(), DWORD(info->ExceptionRecord->ExceptionCode));
  return EXCEPTION_CONTINUE_SEARCH; // not reached
}
#endif

}

void install_crash_handlers(void (*restore_fatal_state)()) {
  if (restore_fatal_state) {
    fatal_restore = restore_fatal_state;
  }
  static bool installed = false;
  if (installed) {
    return;
  }
  installed = true;
  std::set_terminate(on_terminate);
#if defined(_WIN32)
  ::SetUnhandledExceptionFilter(unhandled_exception_filter);
#endif
}

void log_fatal_signal(int sig) {
  if (not is_crash_signal(sig) or crash_reported) {
    return;
  }
  RawWriter writer;
  writer.append("\n[tui++ fatal] signal ");
  writer.append(signal_name(sig));
  writer.append(" (");
  writer.append_dec(sig);
  writer.append(")\n");
  writer.append_backtrace();
  writer.flush();
  crash_reported = 1;
}

}
