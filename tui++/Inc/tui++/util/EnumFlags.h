#pragma once

#include <type_traits>

namespace tui::util {

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
class EnumFlags {
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

    friend class EnumFlags;
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

    constexpr const_iterator& operator++() {
      this->bitset ^= (this->bitset & -this->bitset);
      return *this;
    }

    constexpr E operator*() const {
      return E(UT(1) << __builtin_ctzl(this->bitset));
    }
  };

public:
  constexpr EnumFlags() :
      flags { 0 } {
  }

  constexpr EnumFlags(E e) :
      flags { UT(e) } {
  }

  constexpr EnumFlags(const EnumFlags &other) = default;

  constexpr EnumFlags& operator=(const EnumFlags &other) = default;

public:
  constexpr EnumFlags& operator|=(E other) {
    this->flags |= UT(other);
    return *this;
  }

  constexpr EnumFlags operator|(E other) const {
    auto result = *this;
    result |= other;
    return result;
  }

  constexpr EnumFlags& operator|=(const EnumFlags &other) {
    this->flags |= other.flags;
    return *this;
  }

  constexpr EnumFlags operator|(const EnumFlags &other) const {
    auto result = *this;
    result |= other;
    return result;
  }

  constexpr EnumFlags operator~() const {
    return {E(~this->flags)};
  }

  constexpr EnumFlags operator&=(E other) {
    this->flags &= UT(other);
    return *this;
  }

  constexpr EnumFlags operator&(E other) const {
    auto result = *this;
    result &= other;
    return result;
  }

  constexpr EnumFlags operator&=(const EnumFlags &other) {
    this->flags &= other.flags;
    return *this;
  }

  constexpr EnumFlags operator&(const EnumFlags &other) const {
    auto result = *this;
    result &= other;
    return result;
  }

  constexpr operator bool() const {
    return bool(this->flags);
  }

  constexpr bool operator==(const EnumFlags &other) const {
    return this->flags == other.flags;
  }

  constexpr bool operator==(const E &other) const {
    return this->flags == UT(other);
  }

  constexpr bool operator!=(const EnumFlags &other) {
    return this->flags != other.flags;
  }

  constexpr bool operator!=(const E &other) const {
    return this->flags != UT(other);
  }

  constexpr const_iterator begin() const {
    return {this->flags};
  }

  constexpr const_iterator end() const {
    return {};
  }
};

}

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
constexpr tui::util::EnumFlags<E> operator|(E a, E b) {
  return {E(std::underlying_type_t<E>(a) | std::underlying_type_t<E>(b))};
}

template<typename E, std::enable_if_t<std::is_enum_v<E>, bool> = true>
constexpr tui::util::EnumFlags<E> operator&(E a, E b) {
  return {E(std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b))};
}
