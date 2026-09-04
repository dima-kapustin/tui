#pragma once

#include <string>
#include <ostream>
#include <typeinfo>

#include <cstddef>
#include <cstring>
#include <vector>

#if __has_include(<cxxabi.h>)
# include <cxxabi.h>
#endif

namespace tui::util {

// Demangles a (possibly mangled) name returned by typeid, e.g. "N3tui6MenuBarE"
// to "tui::MenuBar". Names that are not mangled are returned unchanged.
//
// __cxa_demangle is given a buffer owned by this function: when the name fits
// it writes into that buffer and returns it, so the only pointer ever freed
// here is our own. When the name does not fit, the runtime allocates a larger
// buffer of its own; the demangled text is still copied out, but that runtime
// buffer is deliberately not freed -- freeing it with the C library's free()
// is not safe when the runtime and the application live on different heaps
// (msys2's libstdc++ DLL on Windows).
inline std::string demangle(const char *name) {
#if __has_include(<cxxabi.h>)
  auto length = std::strlen(name);
  auto buffer = std::vector<char>(length * 4 + 128);
  for (;;) {
    auto capacity = buffer.size();
    auto used = capacity;
    auto status = 0;
    auto *result = abi::__cxa_demangle(name, buffer.data(), &used, &status);
    if (status == 0 and result == buffer.data()) {
      return std::string(result);
    }
    if (status != 0 or not result) {
      break; // not a mangled name; the caller keeps `name` as-is
    }
    // The name did not fit: grow the buffer and retry. The buffer the runtime
    // allocated for `result` stays with the runtime (see above).
    buffer.resize(capacity * 2);
  }
#endif
  return name;
}

}

namespace std {

inline ostream& operator<<(ostream &os, const type_info &type_info) {
  return os << tui::util::demangle(type_info.name());
}

inline string to_string(const type_info &type_info) {
  return tui::util::demangle(type_info.name());
}

}
