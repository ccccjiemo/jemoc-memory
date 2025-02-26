//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_TRAITS_CHECK_H
#define JEMOC_STREAM_TEST_TRAITS_CHECK_H
#include "napi/native_api.h"
#include <type_traits>
#include <string>

namespace traits_check {
// GetClassName 检查
template <typename, typename = void> struct HasGetClassName : std::false_type {};

template <typename T>
struct HasGetClassName<T, std::void_t<decltype(T::GetClassName())>>
    : std::is_convertible<decltype(T::GetClassName()), std::string> {};

// ConstructInstance 检查
template <typename, typename = void> struct HasConstructInstance : std::false_type {};

template <typename T>
struct HasConstructInstance<T, std::void_t<decltype(T::ConstructInstance(
                                   std::declval<napi_env>(), std::declval<size_t>(), std::declval<napi_value *>()))>>
    : std::true_type {};

// GetMethods 检查
template <typename, typename = void> struct HasGetMethods : std::false_type {};

template <typename T>
struct HasGetMethods<T, std::void_t<decltype(T::GetMethods(std::declval<napi_env>()))>>
    : std::is_convertible<decltype(T::GetMethods(std::declval<napi_env>())), std::vector<napi_property_descriptor>> {};

// 综合检查器
template <typename T> struct ValidateTraits {
    static constexpr bool value =
        HasGetClassName<T>::value && HasConstructInstance<T>::value && HasGetMethods<T>::value;

    static_assert(HasGetClassName<T>::value, "Traits must implement static std::string GetClassName()");
    static_assert(HasConstructInstance<T>::value,
                  "Traits must implement static InstanceType ConstructInstance(napi_env, size_t, napi_value*)");
    static_assert(HasGetMethods<T>::value,
                  "Traits must implement static std::vector<napi_property_descriptor> GetMethods()");
};
} // namespace traits_check

#endif //JEMOC_STREAM_TEST_TRAITS_CHECK_H
