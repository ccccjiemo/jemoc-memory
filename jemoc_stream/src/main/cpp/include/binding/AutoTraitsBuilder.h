#include <napi/native_api.h>
#include <type_traits>



// 核心声明宏（支持链式调用）
#define DECLARE_BINDING(ClassName, Derived)                                                                            \
public:                                                                                                                \
    struct Traits : public DefaultJSBindingTraits<Derived> {                                                           \
        using Base = DefaultJSBindingTraits<Derived>;                                                                  \
        static constexpr auto GetClassName() { return #ClassName; }

#define INHERITS(ParentClass)                                                                                          \
    static constexpr auto GetParentName() { return ParentClass; }

#define ABSTRACT_CLASS                                                                                                 \
    static constexpr bool IsAbstractClass() { return true; }

#define METHODS(...)                                                                                                   \
    static auto GetMethods(napi_env env) { return std::vector<napi_property_descriptor>{__VA_ARGS__}; }

#define CONSTRUCTOR(Func)                                                                                              \
    static auto ConstructInstance(napi_env env, size_t argc, napi_value *argv) { return Func(env, argc, argv); }

#define REMOVE_WRAPPER(Func)                                                                                           \
    static void RemoveWrapper(napi_env env, NapiWrapperType *wrapper) { Func(env, wrapper); }

#define END_BINDING                                                                                                    \
    }                                                                                                                  \
    ;                                                                                                                  \
    using TraitsType = Traits; /* 类型别名用于IDE提示 */                                                       \
    /**/

// 方法定义宏（支持重载检测）
#define METHOD(Name, Func)                                                                                             \
    [] {                                                                                                               \
        using FuncType = decltype(&Func);                                                                              \
        static_assert(traits_check::IsValidMethod<FuncType>::value, "Invalid method signature");                       \
        return napi_property_descriptor{#Name, nullptr, Func, nullptr, nullptr, nullptr, napi_default, nullptr};       \
    }()

// endregion