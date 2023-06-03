#include <array>
#include <string>
#include <stdexcept>

#ifdef __APPLE__
#    include <cwchar>
#else
#    include <cuchar>
#endif

namespace tui {

char32_t mb_to_u32(std::array<char, 4> bytes) {
  auto result = char32_t { };
  auto mb_state = std::mbstate_t { };
#ifdef __APPLE__
  static_assert(sizeof(wchar_t) == sizeof(char32_t));
  static_assert(alignof(wchar_t) == alignof(char32_t));
  auto const error = std::mbrtowc(reinterpret_cast<wchar_t*>(&result), bytes.data(), 4, &mb_state);
#else
  auto const error = std::mbrtoc32(&result, bytes.data(), 4, &mb_state);
#endif
  if (error == std::size_t(-1)) {
    throw std::runtime_error { "Illegal byte sequence" };
  } else if (error == std::size_t(-2)) {
    throw std::runtime_error { "Incomplete byte sequence" };
  }
  return result;
}

std::string u32_to_mb(char32_t c) {
  auto result = std::array<char, 4> { };
  auto state = std::mbstate_t { };

#ifdef __APPLE__
  static_assert(sizeof(wchar_t) == sizeof(char32_t));
  static_assert(alignof(wchar_t) == alignof(char32_t));
  auto count = std::wcrtomb(result.data(), wchar_t(c), &state);
#else
  auto count = std::c32rtomb(result.data(), c, &state);
#endif
  if (count == size_t(-1)) {
    throw std::runtime_error { "Illegal byte sequence" };
  }
  return {result.data(), count};
}

}
