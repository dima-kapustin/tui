#pragma once

#include <type_traits>

namespace tui::util {

template<typename E, typename = void>
class EnumFlags;

template<typename E>
class EnumFlags<E, std::enable_if_t<std::is_enum_v<E>>> {
  using U = std::underlying_type_t<E>;

  U flags;

public:
  constexpr static EnumFlags NONE = { };

public:
  class const_iterator {
    U bitset;

  protected:
    const_iterator(U bitset) :
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
      return E(U(1) << __builtin_ctzl(this->bitset));
    }
  };

public:
  constexpr EnumFlags() :
      flags { 0 } {
  }

  constexpr EnumFlags(E e) :
      flags { U(e) } {
  }

  constexpr EnumFlags(const EnumFlags &other) = default;

  constexpr EnumFlags& operator=(const EnumFlags &other) = default;

public:
  constexpr EnumFlags& operator|=(E other) {
    this->flags |= U(other);
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
    this->flags &= U(other);
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
    return this->flags == U(other);
  }

  constexpr bool operator!=(const EnumFlags &other) {
    return this->flags != other.flags;
  }

  constexpr bool operator!=(const E &other) const {
    return this->flags != U(other);
  }

  constexpr const_iterator begin() const {
    return {this->flags};
  }

  constexpr const_iterator end() const {
    return {};
  }
};

}

template<typename E>
requires (std::is_enum_v<E>)
constexpr tui::util::EnumFlags<E> operator|(E a, E b) {
  return {E(std::underlying_type_t<E>(a) | std::underlying_type_t<E>(b))};
}

template<typename E>
requires (std::is_enum_v<E>)
constexpr tui::util::EnumFlags<E> operator&(E a, E b) {
  return {E(std::underlying_type_t<E>(a) & std::underlying_type_t<E>(b))};
}

template<typename E>
requires (std::is_enum_v<E>)
constexpr tui::util::EnumFlags<E> operator~(E a) {
  return {E(~std::underlying_type_t<E>(a))};
}
