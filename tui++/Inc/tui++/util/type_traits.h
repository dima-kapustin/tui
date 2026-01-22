#pragma once

#include <memory>
#include <optional>
#include <type_traits>

namespace tui::util {

template<typename T>
struct is_shared_ptr: std::false_type {
};

template<typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {
};

template<typename T>
constexpr bool is_shared_ptr_v = is_shared_ptr<T>::value;

template<typename T>
struct is_unique_ptr: std::false_type {
};

template<typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {
};

template<typename T>
constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

template<typename T>
struct is_weak_ptr: std::false_type {
};

template<typename T>
struct is_weak_ptr<std::weak_ptr<T>> : std::true_type {
};

template<typename T>
constexpr bool is_weak_ptr_v = is_weak_ptr<T>::value;

template<typename T>
constexpr bool is_smart_pointer_v = is_shared_ptr_v<T> or is_unique_ptr_v<T> or is_weak_ptr_v<T>;

namespace detail {
template <typename T>
struct is_optional : std::false_type {};

template <typename U>
struct is_optional<std::optional<U>> : std::true_type {};
}

template <typename T>
using is_optional = detail::is_optional<std::remove_cvref_t<T>>;

template <typename T>
constexpr bool is_optional_v = is_optional<T>::value;

}
