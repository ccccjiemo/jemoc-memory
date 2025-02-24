//
// Created on 2025/2/23.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_INSTANCEPOLICY_H
#define JEMOC_STREAM_TEST_INSTANCEPOLICY_H
#include <memory>

template <typename T> struct SharedPtrStoragePolicy {
    using InstanceType = std::shared_ptr<T>;
    using AccessHandle = std::shared_ptr<T>;

    template <typename... Args> static InstanceType Create(Args &&...args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    static void Destroy(InstanceType &) {} // shared_ptr自动管理

    static T *GetPointer(const InstanceType &instance) noexcept { return instance.get(); }

    static InstanceType GetShared(const InstanceType &instance) { return instance; }
};

template <typename T> struct UniquePtrStoragePolicy {
    using InstanceType = std::unique_ptr<T>;

    template <typename... Args> static InstanceType Create(Args &&...args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    static void Destroy(InstanceType &) {} // unique_ptr自动释放

    static T *GetPointer(const InstanceType &instance) noexcept { return instance.get(); }

    template <typename U = void> static U GetShared(InstanceType) = delete;
};

template <typename T> struct RawPtrStoragePolicy {
    using InstanceType = T *;

    template <typename... Args> static InstanceType Create(Args &&...args) {
        return new T(std::forward<Args>(args)...);
    }

    static void Destroy(InstanceType instance) { delete instance; }

    static T *GetPointer(const InstanceType &instance) { return instance; }

    static std::shared_ptr<T> GetShared(const InstanceType &instance) { return std::shared_ptr<T>(instance); }
};


#endif // JEMOC_STREAM_TEST_INSTANCEPOLICY_H
