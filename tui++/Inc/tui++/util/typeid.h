#pragma once

#include <string>
#include <ostream>
#include <typeinfo>

#if __has_include(<cxxabi.h>)
# include <cxxabi.h>
# include <cstdlib>
# include <memory>
#endif

namespace tui::util {

namespace detail {

constexpr bool CXXABI {
#if __has_include(<cxxabi.h>)
  true
#endif
};

}

auto demangle(const char *name, size_t *length = nullptr) noexcept (not detail::CXXABI) {
  if constexpr (detail::CXXABI) {
    auto dmg = abi::__cxa_demangle(name, nullptr, length, nullptr);
    return std::unique_ptr<char, decltype(std::free)*>(dmg, std::free);
  } else {
    return name;         // NOP: assume already demangled if not on CXXABI
  }
}

}

namespace std {

inline ostream& operator<<(ostream &os, const type_info &type_info) {
  return os << tui::util::demangle(type_info.name());
}

constexpr string to_string(const type_info &type_info) {
  size_t length = 0;
  if (auto name = tui::util::demangle(type_info.name(), &length)) {
    return {name.get(), length};
  }
  return {};
}

}
