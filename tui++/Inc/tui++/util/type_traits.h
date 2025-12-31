#pragma once

#include <memory>
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

}
