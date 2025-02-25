////
//// Created on 2025/2/25.
////
//// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
//// please include "napi/native_api.h".
//
//#ifndef JEMOC_STREAM_TEST_TRAITSBUILD_H
//#define JEMOC_STREAM_TEST_TRAITSBUILD_H
//#include <napi/native_api.h>
//#include <string>
//
//template <size_t N> struct FixedString {
//    constexpr FixedString(const char (&str)[N]) { std::copy_n(str, N, data); }
//    char data[N];
//};
//
//
//enum MethodType {
//    MethodType_DefaultMethod,
//    MethodType_StaticMethod,
//    MethodType_Constant,
//    MethodType_Getter,
//    MethodType_Setter,
//    MethodType_Accessor
//};
//template <auto Method, FixedString Name, MethodType Type> struct MethodMeta {
//    static constexpr auto type = Type;
//    static constexpr auto method = Method;
//    static constexpr auto name = Name;
//};
//
//template <typename Derived, FixedString ClassName, FixedString ParentName = FixedString(""),
//          auto ConstructorPtr = nullptr, typename... Methods>
//class TraitsBuilder {
//public:
//    using JSBindingType = typename Derived::JSBindingType;
//    using InstanceType = typename JSBindingType::InstanceType;
//
//#define ADD_METHOD(type, method, name_str)                                                                             \
//    AddMethod<method,                                                                                                  \
//              [] {                                                                                                     \
//                  constexpr FixedString str(name_str);                                                                 \
//                  return str;                                                                                          \
//              }(),                                                                                                     \
//              MethodType_##type>()
//
//    template <auto Method, FixedString Name, MethodType Type> auto AddMethod() const {
//        return TraitsBuilder<Derived, ClassName, ParentName, ConstructorPtr, Methods...,
//                             MethodMeta<Method, Name, Type>>();
//    }
//
//#define ADD_STATIC_METHOD(method, name) ADD_METHOD(StaticMethod, method, name)
//#define ADD_DEFAULT_METHOD(method, name) ADD_METHOD(DefaultMethod, method, name)
//#define ADD_CONSTANT(method, name) ADD_METHOD(Constant, method, name)
//#define ADD_GETTER(method, name) ADD_METHOD(Getter, method, name)
//#define ADD_SETTER(method, name) ADD_METHOD(Setter, method, name)
//#define ADD_ACCESSOR(method, name) ADD_METHOD(Accessor, method, name)
//
//    // template <auto Method, const char *Name, const MethodType Type> auto AddMethod() const {
//    //     return TraitsBuilder<Derived, ClassName, ParentName, ConstructorPtr, Methods...,
//    //                          MethodMeta<Method, Name, Type>>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddStaticMethod() const {
//    //     return AddMethod<Method, Name, MethodType_StaticMethod>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddDefaultMethod() const {
//    //     return AddMethod<Method, Name, MethodType_StaticMethod>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddConstant() const {
//    //     return AddMethod<Method, Name, MethodType_Constant>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddGetter() const {
//    //     return AddMethod<Method, Name, MethodType_Getter>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddSetter() const {
//    //     return AddMethod<Method, Name, MethodType_Setter>();
//    // }
//
//    // template <auto Method, const char *Name> auto AddAccessor() const {
//    //     return AddMethod<Method, Name, MethodType_Accessor>();
//    // }
//
//    template <FixedString NewParentName> auto Parent() const {
//        return TraitsBuilder<Derived, ClassName, NewParentName, ConstructorPtr, Methods...>();
//    }
//
//    template <auto NewConstructor> auto Constructor() const {
//        return TraitsBuilder<Derived, ClassName, ParentName, NewConstructor, Methods...>();
//    }
//
//    struct Traits {
//        static std::string GetClassName() { return ClassName.data; }
//        static std::string GetParentName() { return ParentName ? ParentName : ""; }
//        static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) {
//            if constexpr (ConstructorPtr != nullptr) {
//                return ConstructorPtr(env, argc, argv);
//            }
//            return nullptr;
//        }
//        static std::vector<napi_property_descriptor> GetMethods(napi_env env) {}
//    };
//
//private:
//    template <typename Meta>
//    static void AddMethodDescriptor(napi_env env, std::vector<napi_property_descriptor> &out) {}
//};
//
//#endif // JEMOC_STREAM_TEST_TRAITSBUILD_H
