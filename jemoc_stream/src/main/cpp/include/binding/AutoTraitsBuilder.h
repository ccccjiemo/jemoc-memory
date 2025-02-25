#include <napi/native_api.h>
#include <type_traits>

// region 宏定义：Traits 构建工具

// 基础声明宏（必填类名，可选父类名和抽象标志）
#define DECLARE_BINDING(ClassName, ParentName,...)                                                                                \
    static std::string GetClassName() { return #ClassName; }                                                           \
    static std::string GetParentName() { return ParentName; }                                        \
    static bool IsAbstractClass() { return GET_ABSTRACT_FLAG(__VA_ARGS__, false); }

// 方法列表包裹宏
#define DECLARE_METHODS_START                                                                                          \
    static std::vector<napi_property_descriptor> GetMethods(napi_env env) {                                            \
        return

#define DECLARE_METHODS_END                                                                                            \
    ;                                                                                                                  \
    }

//// 单个方法定义宏（支持重载检测）
// #def ine DEFINE_METHOD(Name, Method) \
//    DECLARE_METHOD_IMPL(Name, Method, decltype(Method))

// 构造函数绑定宏
#define DEFINE_CONSTRUCTOR(Func)                                                                                       \
    static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) { return Func(env, argc, argv); }

// 自定义 RemoveWrapper 宏
#define DEFINE_REMOVE_WRAPPER(Func)                                                                                    \
    static void RemoveWrapper(napi_env env, NapiWrapperType *wrapper) { Func(env, wrapper); }

// endregion

// region 辅助宏（参数解析与重载检测）

// 提取父类名（支持可选参数）
#define GET_PARENT_NAME(...) GET_PARENT_NAME_IMPL(__VA_ARGS__, "")
#define GET_PARENT_NAME_IMPL(Parent, ...) #Parent

// 提取抽象标志（支持可选参数）
#define GET_ABSTRACT_FLAG(...) GET_ABSTRACT_FLAG_IMPL(__VA_ARGS__, false)
#define GET_ABSTRACT_FLAG_IMPL(Flag, ...) Flag

// 方法定义实现（类型安全检查）
#define DECLARE_METHOD_IMPL(Name, Method, Signature)                                                                   \
    []() {                                                                                                             \
        using MethodType = std::decay_t<decltype(Method)>;                                                             \
        static_assert(std::is_invocable_r_v<napi_value, MethodType, NativeType *, napi_env, size_t, napi_value *> ||   \
                          std::is_invocable_r_v<napi_value, MethodType, NativeType *, napi_env>,                       \
                      "Method signature must be: napi_value Method(T*, napi_env, size_t, napi_value*) or napi_value "  \
                      "Method(T*, napi_env)");                                                                         \
        return napi_property_descriptor{Name, nullptr, Method, nullptr, nullptr, nullptr, napi_default, nullptr};      \
    }()

// endregion