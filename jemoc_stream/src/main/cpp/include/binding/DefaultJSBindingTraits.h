//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_DEFAULTJSBINDINGTRAITS_H
#define JEMOC_STREAM_TEST_DEFAULTJSBINDINGTRAITS_H
#include "binding/traits_check.h"
#include "napi/native_api.h"
#include <string>

/**
 * @brief DefaultJSBindingTraits 是一个模板类，用于定义 JavaScript 绑定的默认行为。
 *
 * 用户需要继承此模板并提供特定类型的实现。以下是对该模板的详细说明：
 *
 * ### 必须实现的方法
 * - **static std::string GetClassName()**
 *   返回绑定类的名称。此方法没有默认实现，用户必须提供具体实现。
 *
 * ### 可选方法（已提供默认实现）
 * 以下方法已经提供了默认实现，但用户可以根据需要重写它们：
 *
 * - **static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv)**
 *   构造绑定类的实例。默认返回 `nullptr`，表示不支持构造实例。
 *
 * - **static std::string GetParentName()**
 *   返回父类的名称。默认返回空字符串，表示没有父类。
 *
 * - **static bool IsAbstractClass()**
 *   判断当前类是否为抽象类。默认返回 `false`。
 *
 * - **static std::vector<napi_property_descriptor> GetMethods()**
 *   返回绑定类的方法列表。默认返回空列表，表示没有方法。
 *
 * - **static void RemoveWrapper(napi_env env, NapiWrapperType *wrapper)**
 *   移除与实例关联的包装器时调用的方法，用于销毁前的额外操作，比如数据保存。默认实现为空操作。
 *
 * - **template <typename E = ExtraData> static auto CreateExtraData(napi_env env, const InstanceType &instance)**
 *   创建额外数据（ExtraData）。如果 `ExtraData` 不是 `void` 类型，则默认返回其默认构造的对象。
 *
 * ### 使用方式
 * 用户需要通过继承 `DefaultJSBindingTraits` 并指定 `Derived` 类型来使用此模板。例如：
 *
 * ```cpp
 * struct MyBindingTraits : public DefaultJSBindingTraits<MyBindingTraits> {
 *     static std::string GetClassName() { return "MyClass"; }
 * };
 * ```
 */

template <typename Derived> struct DefaultJSBindingTraits {
    static_assert(traits_check::ValidateTraits<Derived>::value, "Invalid Traits");
    using JSBindingType = typename Derived::JSBindingType;
    using T = typename JSBindingType::NativeType;
    using StoragePolicy = typename JSBindingType::StoragePolicyType;
    using InstanceType = typename StoragePolicy::InstanceType;
    using ExtraData = typename JSBindingType::ExtraDataType;
    using NapiWrapperType = typename JSBindingType::NapiWrapperType;

    // 必须由用户实现的方法
    // static std::string GetClassName();

    // 可选方法，默认实现
    static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) { return nullptr; }
    static std::string GetParentName() { return ""; }
    static bool IsAbstractClass() { return false; }
    static std::vector<napi_property_descriptor> GetMethods(napi_env env) { return {}; }

    static void SetNameSpacePath(const std::string nsName) { NameSpacePath = nsName; }
    static std::string GetNameSpacePath() { return NameSpacePath; }
    static void RemoveWrapper(napi_env env, NapiWrapperType *wrapper) {}

    template <typename E = ExtraData>
    static auto CreateExtraData(napi_env env, const InstanceType &instance) -> std::enable_if_t<!std::is_void_v<E>, E> {
        return E(); // 默认构造ExtraData
    }

private:
    static inline std::string NameSpacePath = "";
};

#endif // JEMOC_STREAM_TEST_DEFAULTJSBINDINGTRAITS_H
