#pragma once

#include <type_traits>

namespace tui {

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
class EnumMask {
  using UT = std::underlying_type_t<E>;

  UT flags;

public:
  constexpr static auto NONE = E(0);

public:
  class const_iterator {
    UT bitset;

  protected:
    const_iterator(UT bitset) :
        bitset(bitset) {
    }

    friend class EnumMask;
  public:
    constexpr const_iterator() :
        bitset(0) {
    }

    constexpr const_iterator(const const_iterator&) = default;

    constexpr bool operator==(const const_iterator &other) const {
      return this->bitset == other.bitset;
    }

    constexpr bool operator!=(const const_iterator &other) const {
      return this->bitset != other.bitset;
    }

    const_iterator& operator++() {
      this->bitset ^= (this->bitset & -this->bitset);
      return *this;
    }

    E operator*() const {
      return E(UT(1) << __builtin_ctzl(this->bitset));
    }
  };

public:
  constexpr EnumMask() :
      flags { 0 } {
  }

  constexpr EnumMask(E e) :
      flags { UT(e) } {
  }

  constexpr EnumMask(const EnumMask &other) = default;

  constexpr EnumMask& operator=(const EnumMask &other) = default;

public:
  constexpr EnumMask& operator|=(E other) {
    this->flags |= UT(other);
    return *this;
  }

  constexpr EnumMask operator|(E other) const {
    auto result = *this;
    result |= other;
    return result;
  }

  constexpr EnumMask& operator|=(const EnumMask &other) {
    this->flags |= other.flags;
    return *this;
  }

  constexpr EnumMask operator|(const EnumMask &other) const {
    auto result = *this;
    result |= other;
    return result;
  }

  constexpr EnumMask operator~() const {
    return {E(~this->flags)};
  }

  constexpr EnumMask operator&=(E other) {
    this->flags &= UT(other);
    return *this;
  }

  constexpr EnumMask operator&(E other) const {
    auto result = *this;
    result &= other;
    return result;
  }

  constexpr EnumMask operator&=(const EnumMask &other) {
    this->flags &= other.flags;
    return *this;
  }

  constexpr EnumMask operator&(const EnumMask &other) const {
    auto result = *this;
    result &= other;
    return result;
  }

  constexpr operator bool() const {
    return bool(this->flags);
  }

  constexpr bool operator==(const EnumMask &other) const {
    return this->flags == other.flags;
  }

  constexpr bool operator==(const E &other) const {
    return this->flags == UT(other);
  }

  constexpr bool operator!=(const EnumMask &other) {
    return this->flags != other.flags;
  }

  constexpr bool operator!=(const E &other) const {
    return this->flags != UT(other);
  }

  const_iterator begin() const {
    return {this->flags};
  }

  const_iterator end() const {
    return {};
  }
};

}

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
constexpr tui::EnumMask<E> operator|(E a, E b) {
  return {E(std::underlying_type_t<E>(a) | std::underlying_type_t<E>(b))};
}

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
constexpr tui::EnumMask<E> operator&(E a, E b) {
  return {E(std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b))};
}
