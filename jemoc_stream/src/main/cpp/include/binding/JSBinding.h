//
// Created on 2025/2/23.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_JSBINDING_H
#define JEMOC_STREAM_TEST_JSBINDING_H

#include "StoragePolicy.h"
#include "binding/DefaultJSBindingTraits.h"
#include "napi/native_api.h"
#include "traits_check.h"
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename T, typename Derived, typename ExtraData = void,
          template <typename> class StoragePolicy = SharedPtrStoragePolicy>
class JSBinding {
public:
    using NativeType = T;
    using StoragePolicyType = StoragePolicy<T>;
    using JSBindingType = JSBinding<T, Derived, ExtraData, StoragePolicy>;
    using DerivedType = Derived;
    using InstanceType = typename StoragePolicyType::InstanceType;
    using SafeExtraDataType = std::conditional_t<std::is_void_v<ExtraData>, std::nullptr_t, ExtraData>;
    using ExtraDataType = SafeExtraDataType;


#define NAPI_CALL(env, func) NAPI_CALL_BASE(env, func, __LINE__)

#define NAPI_CALL_BASE(env, func, line)                                                                                \
    if (napi_ok != func) {                                                                                             \
        napi_throw_error(env, "NAPI_CALL_ERROR", #func);                                                               \
    }

#ifndef DEFINE_DEFAULT_METHOD
#define DEFINE_DEFAULT_METHOD

    static napi_value ExportNamespace(napi_env env, napi_value exports, const std::string &nsName) {
        napi_value nsObject;
        NAPI_CALL(env, napi_get_named_property(env, exports, nsName.c_str(), &nsObject))
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, nsObject, &type))
        if (type != napi_object) {
            NAPI_CALL(env, napi_create_object(env, &nsObject))
            NAPI_CALL(env, napi_set_named_property(env, exports, nsName.c_str(), nsObject))
        }
        return nsObject;
    }

    template <typename ObjectType>
    static void ExportObject(napi_env env, napi_value exports, const std::string &name, const ObjectType obj) {
        napi_value jsObject = ConvertToJSObject(env, obj);
        NAPI_CALL(env, napi_set_named_property(env, exports, name.c_str(), jsObject))
    }

    template <typename ConstantType>
    static void ExportConstant(napi_env env, napi_value exports, const std::string &name, const ConstantType &value) {
        NAPI_CALL(env, napi_set_name_property(env, exports, name.c_str(), ToNapiValue(env, value)))
    }

    static void ExportTopLevel(napi_env env, napi_value exports) { Init(env, exports); }

#endif // DEFINE_DEFAULT_METHOD

#ifndef DEFINE_NAPI_WRAPPER
#define DEFINE_NAPI_WRAPPER

    template <typename E, typename InstType> struct NapiWrapperBase {
        E extraData;
        InstType instance;
        NapiWrapperBase(InstType inst, E extra) : instance(std::move(inst)), extraData(std::move(extra)) {}
    };

    template <typename InstType> struct NapiWrapperBase<void, InstType> {
        InstType instance;
        explicit NapiWrapperBase(InstType inst) : instance(std::move(inst)) {}
    };

    struct NapiWrapper : NapiWrapperBase<ExtraData, InstanceType> {
        using Base = NapiWrapperBase<ExtraData, InstanceType>;

        template <typename... Args>
        NapiWrapper(InstanceType instance, Args &&...args) : Base(std::move(instance), std::forward<Args>(args)...) {}

        ~NapiWrapper() { StoragePolicyType::Destroy(this->instance); }

        T *GetNative() const noexcept { return StoragePolicyType::GetPointer(this->instance); }

        template <typename SP = StoragePolicyType> auto GetShared() const -> decltype(SP::GetShared(this->instance)) {
            return SP::GetShared(this->instance);
        }

        template <typename E = ExtraData> std::enable_if_t<!std::is_void<E>::value, E &> GetExtraData() noexcept {
            return this->extraData;
        }
    };

    using NapiWrapperType = NapiWrapper;

    struct JSCallbackInfo {
        napi_env env;
        std::vector<napi_value> argv;
        napi_value thisArgs;
        NapiWrapperType *wrapper;
    };


#endif // DEFINE_NAPI_WRAPPER

#ifndef DEFINE_ASYNCWORKER
#define DEFINE_ASYNCWORKER

    struct AsyncContext {
        napi_env env;
        napi_deferred deferred;
        napi_async_work work;
        T *native;
        std::function<void()> executor;
        std::function<void(napi_env)> resolver;
        std::function<void(napi_env, napi_value)> rejecter;
    };

    template <auto Method> static napi_value AsyncMethodWrapper(napi_env env, napi_callback_info info) {
        napi_value promise;
        AsyncContext *context = new AsyncContext{env};

        NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise))

        try {
            size_t argc = 0;
            napi_value thisArgs;
            NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, &thisArgs, nullptr));

//            context->native = GetNativeInstance(env, thisArgs);
            NapiWrapper *wrapper = GetWrapper(env, thisArgs);
            context->native = wrapper->GetNative();

            context->executor = [=] {
                if constexpr (!std::is_void_v<ExtraDataType> &&
                              std::is_invocable_v<decltype(Method), T *, ExtraDataType &, napi_env>) {
                    Method(context->native, wrapper->GetExtraData(), env);
                } else if constexpr (std::is_invocable_v<decltype(Method), T *, napi_env>) {
                    Method(context->native, env);
                } else if constexpr (!std::is_void_v<ExtraDataType> &&
                                     std::is_invocable_v<decltype(Method), T *, ExtraDataType &>) {
                    Method(context->native, wrapper->GetExtraData());
                } else {
                    Method(context->native);
                }
            };

            napi_value resource_name;
            NAPI_CALL(env, napi_create_string_utf8(env, "AsyncWork", NAPI_AUTO_LENGTH, &resource_name))
            NAPI_CALL(env, napi_create_async_work(
                               env, nullptr, resource_name,
                               [](napi_env env, void *data) {
                                   AsyncContext *ctx = static_cast<AsyncContext *>(data);
                                   ctx->executor();
                               },
                               [](napi_env env, napi_status status, void *data) {
                                   AsyncContext *ctx = static_cast<AsyncContext *>(data);
                                   if (status == napi_ok && ctx->resolver) {
                                       napi_value result;
                                       ctx->resolver(env, &result);
                                       NAPI_CALL(env, napi_resolve_deferred(env, ctx->deferred, result))
                                   } else {
                                       napi_value error;
                                       ctx->rejecter(env, &error);
                                       NAPI_CALL(env, napi_reject_deferred(env, ctx->deferred, error))
                                   }
                                   NAPI_CALL(env, napi_delete_async_work(env, ctx->work))
                                   delete ctx;
                               },
                               context, &context->work))
            NAPI_CALL(env, napi_queue_async_work(env, context->work))

        } catch (const std::exception &e) {
            napi_value error;
            NAPI_CALL(env, napi_create_string_utf8(env, e.what(), NAPI_AUTO_LENGTH, &error));
            NAPI_CALL(env, napi_reject_deferred(env, context->deferred, error));
            delete context;
        }
        return promise;
    }


#endif // DEFINE_ASYNCWORKER

    static std::string GetClassName() { return Derived::Traits::GetClassName(); }

    static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) {
        return Derived::Traits::ConstructInstance(env, argc, argv);
    }

    static std::string GetParentName() { return Derived::Traits::GetParentName(); }

    static bool IsAbstractClass() { return Derived::Traits::IsAbstractClass(); }

    static std::vector<napi_property_descriptor> GetMethods(napi_env env) { return Derived::Traits::GetMethods(env); }

    template <typename... Args> static InstanceType CreateInstance(Args... args) {
        return StoragePolicyType::Create(std::forward<Args>(args)...);
    }

    template <typename E = ExtraData>
    static auto CreateExtraData(napi_env env, const InstanceType &instance) ->
        typename std::enable_if<!std::is_same<E, void>::value, E>::type {
        return Derived::Traits::template CreateExtraData<E>(env, instance);
    }

    static napi_value AbstractMethodStub(napi_env env, napi_callback_info) {
        napi_throw_error(env, nullptr, "abstract method must be overridden");
        return nullptr;
    }

    static napi_value DefineClass(napi_env env, const std::string &className, napi_value parent,
                                  const napi_property_descriptor *methods, size_t methodCount,
                                  bool isAbstract = false) {

        napi_callback constructor = isAbstract ? [](napi_env env, napi_callback_info info) -> napi_value {
            napi_value newTarget;
            NAPI_CALL(env, napi_get_new_target(env, info, &newTarget));
            if (newTarget) {
                napi_throw_error(env, nullptr, "cannot instantiate abstract class directly");
                return nullptr;
            }
            return nullptr;
        }
        : Constructor;


        napi_value cons;
        NAPI_CALL(env, napi_define_class(env, className.c_str(), NAPI_AUTO_LENGTH, constructor, nullptr, methodCount,
                                         methods, &cons));

        if (isAbstract) {
            napi_value isAbstractProp;
            NAPI_CALL(env, napi_create_int32(env, 1, &isAbstractProp));
            NAPI_CALL(env, napi_set_named_property(env, cons, "__isAbstract", isAbstractProp));
        }

        if (parent) {
            napi_value baseCon = nullptr;
            napi_value extendCon = nullptr;
            NAPI_CALL(env, napi_get_named_property(env, parent, "prototype", &baseCon))
            NAPI_CALL(env, napi_get_named_property(env, cons, "prototype", &extendCon))
            NAPI_CALL(env, napi_set_named_property(env, extendCon, "__proto__", baseCon))
        }
        return cons;
    }

    static napi_value Constructor(napi_env env, napi_callback_info info) {
        size_t argc = 0;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr))
        std::vector<napi_value> argv(argc);
        napi_value newTarget;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv.data(), &newTarget, nullptr));

        InstanceType instance = ConstructInstance(env, argc, argv.data());
        InstanceBinding(env, newTarget, instance);
        return newTarget;
    }

    static void InstanceBinding(napi_env env, napi_value jsInstance, InstanceType instance) {
        auto deleteFunc = [](napi_env env, void *data, void *hint) {
            NapiWrapper *wrapper = static_cast<NapiWrapper *>(data);
            T *native = wrapper->GetNative();

            {
                std::lock_guard<std::mutex> lock(nativeToJsMutex_);
                auto it = nativeToJsMap_.find(native);
                if (it != nativeToJsMap_.end()) {
                    napi_delete_reference(env, it->second.second);
                    nativeToJsMap_.erase(it);
                }
            }

            Derived::Traits::RemoveWrapper(env, wrapper);
            delete wrapper;
        };
        NapiWrapper *wrapper;
        if constexpr (!std::is_same<ExtraData, void>::value) {
            wrapper = new NapiWrapper(std::move(instance), Derived::Traits::CreateExtraData(env, instance));
        } else {
            wrapper = new NapiWrapper(std::move(instance));
        }
        NAPI_CALL(env, napi_wrap(env, jsInstance, wrapper, deleteFunc, nullptr, nullptr))

        // 存储 Native 指针到弱引用映射
        napi_ref jsRef;
        NAPI_CALL(env, napi_create_reference(env, jsInstance, 0, &jsRef));
        {
            std::lock_guard<std::mutex> lock(nativeToJsMutex_);
            nativeToJsMap_[wrapper->GetNative()] = std::make_pair(env, jsRef);
        }
    }

#ifndef JSBinding_ArgParse
#define JSBinding_ArgParse
    template <typename ArgType, typename Func>
    static ArgType ParseArgImpl(napi_env env, napi_value arg, Func parseFunc) {
        ArgType result;
        napi_status status = parseFunc(env, arg, &result);
        if (status != napi_ok) {
            throw std::runtime_error("Failed to parse argument");
        }
        return result;
    }

    template <typename ArgType> static ArgType ParseArg(napi_env env, napi_value arg) {
        // 默认实现：抛出错误或提示未实现
        throw std::runtime_error("Unsupported type for ParseArg");
    }

    // 特化：处理 int 类型
    template <> static int ParseArg<int>(napi_env env, napi_value arg) {
        return ParseArgImpl<int>(env, arg, napi_get_value_int32);
    }

    // 特化：处理 double 类型
    template <> static double ParseArg<double>(napi_env env, napi_value arg) {
        return ParseArgImpl<double>(env, arg, napi_get_value_double);
    }

    // 特化：处理 std::string 类型
    template <> static std::string ParseArg<std::string>(napi_env env, napi_value arg) {
        size_t length;
        napi_get_value_string_utf8(env, arg, nullptr, 0, &length);
        std::vector<char> buffer(length + 1); // 分配足够的空间，包括终止符
        size_t copied;
        napi_get_value_string_utf8(env, arg, buffer.data(), buffer.size(), &copied);
        return std::string(buffer.data(), copied);
    }

    // 特化：处理 float 类型
    template <> static float ParseArg<float>(napi_env env, napi_value arg) {
        return static_cast<float>(ParseArg<double>(env, arg));
    }

    template <> static bool ParseArg<bool>(napi_env env, napi_value arg) {
        return ParseArgImpl<bool>(env, arg, napi_get_value_bool);
    }

    template <typename ArgType> static napi_value ToNapiValue(napi_env env, const ArgType &value) {
        throw std::runtime_error("Unsupported to napi value type ");
    }

    struct ArrayBufferData {
        void *data;
        size_t length;
        bool isDetached;

        template <typename TData> TData *as() const { return static_cast<TData *>(data); }
    };

    template <> static ArrayBufferData ParseArg<ArrayBufferData>(napi_env env, napi_value arg) {
        ArrayBufferData result;
        napi_status status = napi_get_buffer_info(env, arg, &result.data, &result.length);
        if (status != napi_ok) {
            napi_throw_error(env, nullptr, "Failed to get buffer info");
        }
        result.isDetached = false;
        return result;
    }

    struct TypedArrayInfo {
        void *data;
        size_t length;
        size_t byteOffset;
        size_t byteLength;
        napi_typedarray_type type;
    };

    template <> static TypedArrayInfo ParseArg<TypedArrayInfo>(napi_env env, napi_value arg) {
        TypedArrayInfo result;
        napi_value arrayBuffer;
        NAPI_CALL(env, napi_get_typedarray_info(env, arg, &result.type, &result.length, &result.data, &arrayBuffer,
                                                &result.byteOffset));

        NAPI_CALL(env, napi_get_arraybuffer_info(env, arrayBuffer, nullptr, &result.byteLength));

        return result;
    }

    template <> static napi_value ToNapiValue<int>(napi_env env, const int &value) {
        napi_value result = nullptr;
        napi_create_int32(env, value, &result);
        return result;
    }

    template <> static napi_value ToNapiValue<double>(napi_env env, const double &value) {
        napi_value result = nullptr;
        napi_create_double(env, value, &result);
        return result;
    }

    template <> napi_value ToNapiValue<float>(napi_env env, const float &value) {
        return ToNapiValue<double>(env, static_cast<double>(value));
    }

    template <> static napi_value ToNapiValue<bool>(napi_env env, const bool &value) {
        napi_value result = nullptr;
        napi_get_boolean(env, value, &result);
        return result;
    }

    template <> napi_value ToNapiValue<std::string>(napi_env env, const std::string &value) {
        napi_value result;
        napi_create_string_utf8(env, value.c_str(), value.size(), &result);
        return result;
    }

    template <>
    napi_value ToNapiValue<std::map<std::string, std::string>>(napi_env env,
                                                               const std::map<std::string, std::string> &value) {
        napi_value result = nullptr;
        NAPI_CALL(env, napi_create_object(env, &result));
        for (const auto &pair : value) {
            NAPI_CALL(env, napi_set_named_property(env, result, pair.first.c_str(), ToNapiValue(env, pair.second)))
        }
        return result;
    }

    template <typename TType> static bool As(napi_env env, napi_value value, TType *result) {
        throw std::runtime_error("Unsupported type for ParseArg");
    }

    template <> static bool As<std::string>(napi_env env, napi_value value, std::string *result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type));
        if (type == napi_string) {
            *result = ParseArg<std::string>(env, value);
            return true;
        } else {
            return false;
        }
    }

    template <> static bool As<bool>(napi_env env, napi_value value, bool *result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type))
        if (type == napi_boolean) {
            *result = ParseArg<bool>(env, value);
            return true;
        } else {
            return false;
        }
    }

    template <> static bool As<void *>(napi_env env, napi_value value, void **result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type))
        if (type == napi_object) {
            NAPI_CALL(env, napi_unwrap(env, value, result));
            return *result != nullptr;
        }
        return false;
    }

    // 新增无符号整型支持
    template <> static uint32_t ParseArg<uint32_t>(napi_env env, napi_value arg) {
        return ParseArgImpl<uint32_t>(env, arg, napi_get_value_uint32);
    }

    template <> static napi_value ToNapiValue<uint32_t>(napi_env env, const uint32_t &value) {
        napi_value result;
        napi_create_uint32(env, value, &result);
        return result;
    }

    template <> static bool As<uint32_t>(napi_env env, napi_value value, uint32_t *result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type));
        if (type == napi_number) {
            *result = ParseArg<uint32_t>(env, value);
            return true;
        }
        return false;
    }

    // 新增int64_t支持
    template <> static int64_t ParseArg<int64_t>(napi_env env, napi_value arg) {
        return ParseArgImpl<int64_t>(env, arg, napi_get_value_int64);
    }

    template <> static napi_value ToNapiValue<int64_t>(napi_env env, const int64_t &value) {
        napi_value result;
        napi_create_int64(env, value, &result);
        return result;
    }


    // 新增As对vector的支持
    template <typename TType> static bool As(napi_env env, napi_value value, std::vector<TType> *result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type));
        if (type != napi_object)
            return false;

        bool is_array;
        NAPI_CALL(env, napi_is_array(env, value, &is_array));
        if (!is_array)
            return false;

        try {
            *result = ParseArg<std::vector<TType>>(env, value);
            return true;
        } catch (...) {
            return false;
        }
    }

    // 新增对象转换支持
    template <>
    static napi_value ToNapiValue<std::map<std::string, std::string>>(napi_env env,
                                                                      const std::map<std::string, std::string> &value) {
        napi_value result;
        NAPI_CALL(env, napi_create_object(env, &result));
        for (const auto &pair : value) {
            NAPI_CALL(env, napi_set_named_property(env, result, pair.first.c_str(), ToNapiValue(env, pair.second)));
        }
        return result;
    }

    // 新增nullptr_t支持
    template <> static napi_value ToNapiValue<std::nullptr_t>(napi_env env, const std::nullptr_t &) {
        napi_value result;
        NAPI_CALL(env, napi_get_null(env, &result));
        return result;
    }

    // 新增日期支持
    template <>
    static std::chrono::system_clock::time_point ParseArg<std::chrono::system_clock::time_point>(napi_env env,
                                                                                                 napi_value arg) {
        double time;
        NAPI_CALL(env, napi_get_date_value(env, arg, &time));
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(static_cast<int64_t>(time)));
    }

    template <>
    static napi_value
    ToNapiValue<std::chrono::system_clock::time_point>(napi_env env,
                                                       const std::chrono::system_clock::time_point &value) {
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count();
        napi_value result;
        NAPI_CALL(env, napi_create_date(env, static_cast<double>(millis), &result));
        return result;
    }

    // 新增回调函数支持
    struct CallbackFunction {
        napi_env env;
        napi_ref ref;

        CallbackFunction(napi_env e, napi_value fn) : env(e) {
            NAPI_CALL(env, napi_create_reference(env, fn, 1, &ref));
        }

        ~CallbackFunction() {
            if (ref) {
                NAPI_CALL(env, napi_delete_reference(env, ref));
            }
        }

        napi_value operator()(size_t argc, napi_value *argv) {
            napi_value global, fn, result;
            NAPI_CALL(env, napi_get_global(env, &global));
            NAPI_CALL(env, napi_get_reference_value(env, ref, &fn));
            NAPI_CALL(env, napi_call_function(env, global, fn, argc, argv, &result));
            return result;
        }
    };

    template <> static CallbackFunction ParseArg<CallbackFunction>(napi_env env, napi_value arg) {
        if (!IsFunction(env, arg)) {
            throw std::runtime_error("Argument is not a function");
        }
        return CallbackFunction(env, arg);
    }

    static bool IsFunction(napi_env env, napi_value value) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type));
        return type == napi_function;
    }

// 在 JSBinding_ArgParse 区块内补充下列代码：

// 新增无符号整型支持
    template <> static uint32_t ParseArg<uint32_t>(napi_env env, napi_value arg) {
        return ParseArgImpl<uint32_t>(env, arg, napi_get_value_uint32);
    }

    template <> static napi_value ToNapiValue<uint32_t>(napi_env env, const uint32_t &value) {
        napi_value result;
        napi_create_uint32(env, value, &result);
        return result;
    }

    template <> static bool As<uint32_t>(napi_env env, napi_value value, uint32_t *result) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, value, &type));
        if (type == napi_number) {
            *result = ParseArg<uint32_t>(env, value);
            return true;
        }
        return false;
    }

// 新增int64_t支持
    template <> static int64_t ParseArg<int64_t>(napi_env env, napi_value arg) {
        return ParseArgImpl<int64_t>(env, arg, napi_get_value_int64);
    }

    template <> static napi_value ToNapiValue<int64_t>(napi_env env, const int64_t &value) {
        napi_value result;
        napi_create_int64(env, value, &result);
        return result;
    }

// 新增size_t类型支持
    template <> static size_t ParseArg<size_t>(napi_env env, napi_value arg) {
        return static_cast<size_t>(ParseArg<uint32_t>(env, arg));
    }

// 新增float显式转换
    template <> static float ParseArg<float>(napi_env env, napi_value arg) {
        return static_cast<float>(ParseArg<double>(env, arg));
    }

// 新增nullptr_t支持
    template <> static napi_value ToNapiValue<std::nullptr_t>(napi_env env, const std::nullptr_t &) {
        napi_value result;
        NAPI_CALL(env, napi_get_null(env, &result));
        return result;
    }

// 新增C字符串支持
    template <> static const char *ParseArg<const char *>(napi_env env, napi_value arg) {
        static std::string buffer; // 注意线程安全
        buffer = ParseArg<std::string>(env, arg);
        return buffer.c_str();
    }

// 新增ArrayBuffer支持
    template <> static napi_value ToNapiValue<ArrayBufferData>(napi_env env, const ArrayBufferData &value) {
        napi_value result;
        napi_create_external_arraybuffer(
            env, value.data, value.length, [](napi_env, void *data, void *) {}, nullptr, &result);
        return result;
    }

// 新增TypedArray支持
    template <> static napi_value ToNapiValue<TypedArrayInfo>(napi_env env, const TypedArrayInfo &info) {
        napi_value arrayBuffer;
        void *data = nullptr;
        napi_create_arraybuffer(env, info.byteLength, &data, &arrayBuffer);
        memcpy(data, info.data, info.byteLength);

        napi_value typedArray;
        napi_create_typedarray(env, info.type, info.length, arrayBuffer, 0, &typedArray);
        return typedArray;
    }

// 新增对象键值对支持
    template <>
    static std::map<std::string, std::string> ParseArg<std::map<std::string, std::string>>(napi_env env,
                                                                                           napi_value arg) {
        std::map<std::string, std::string> result;

        napi_value keys;
        NAPI_CALL(env, napi_get_property_names(env, arg, &keys));

        uint32_t length;
        NAPI_CALL(env, napi_get_array_length(env, keys, &length));

        for (uint32_t i = 0; i < length; ++i) {
            napi_value key, value;
            NAPI_CALL(env, napi_get_element(env, keys, i, &key));
            NAPI_CALL(env, napi_get_property(env, arg, key, &value));

            std::string keyStr = ParseArg<std::string>(env, key);
            std::string valueStr = ParseArg<std::string>(env, value);
            result[keyStr] = valueStr;
        }
        return result;
    }

// 新增回调函数包装器
    struct CallbackWrapper {
        napi_env env;
        napi_ref ref;

        CallbackWrapper(napi_env e, napi_value fn) : env(e) { napi_create_reference(env, fn, 1, &ref); }

        ~CallbackWrapper() {
            if (ref)
                napi_delete_reference(env, ref);
        }

        napi_value operator()(size_t argc, const napi_value *argv) {
            napi_value global, fn, result;
            napi_get_global(env, &global);
            napi_get_reference_value(env, ref, &fn);
            napi_call_function(env, global, fn, argc, argv, &result);
            return result;
        }
    };

    template <> static CallbackWrapper ParseArg<CallbackWrapper>(napi_env env, napi_value arg) {
        napi_valuetype type;
        NAPI_CALL(env, napi_typeof(env, arg, &type));
        if (type != napi_function) {
            napi_throw_type_error(env, nullptr, "Function expected");
            return CallbackWrapper(env, nullptr);
        }
        return CallbackWrapper(env, arg);
    }

// 新增日期类型支持（需要包含<chrono>）
    template <>
    static std::chrono::system_clock::time_point ParseArg<std::chrono::system_clock::time_point>(napi_env env,
                                                                                                 napi_value arg) {
        double time;
        NAPI_CALL(env, napi_get_date_value(env, arg, &time));
        return std::chrono::system_clock::time_point(std::chrono::milliseconds(static_cast<int64_t>(time)));
    }

    template <>
    static napi_value
    ToNapiValue<std::chrono::system_clock::time_point>(napi_env env,
                                                       const std::chrono::system_clock::time_point &value) {
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count();
        napi_value result;
        napi_create_date(env, static_cast<double>(millis), &result);
        return result;
    }


#define DEFINE_VECTOR_CONVERSION(TYPE)                                                                                 \
    template <> static std::vector<TYPE> ParseArg<std::vector<TYPE>>(napi_env env, napi_value arg) {                   \
        std::vector<TYPE> result;                                                                                      \
        uint32_t length;                                                                                               \
        NAPI_CALL(env, napi_get_array_length(env, arg, &length));                                                      \
        result.reserve(length);                                                                                        \
                                                                                                                       \
        for (uint32_t i = 0; i < length; ++i) {                                                                        \
            napi_value element;                                                                                        \
            NAPI_CALL(env, napi_get_element(env, arg, i, &element));                                                   \
            result.push_back(ParseArg<TYPE>(env, element));                                                            \
        }                                                                                                              \
        return result;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    template <> static napi_value ToNapiValue<std::vector<TYPE>>(napi_env env, const std::vector<TYPE> &value) {       \
        napi_value array;                                                                                              \
        NAPI_CALL(env, napi_create_array_with_length(env, value.size(), &array));                                      \
        for (size_t i = 0; i < value.size(); ++i) {                                                                    \
            napi_value element = ToNapiValue<TYPE>(env, value[i]);                                                     \
            NAPI_CALL(env, napi_set_element(env, array, i, element));                                                  \
        }                                                                                                              \
        return array;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    template <> static bool As<std::vector<TYPE>>(napi_env env, napi_value value, std::vector<TYPE> * result) {        \
        bool isArray = false;                                                                                          \
        NAPI_CALL(env, napi_is_array(env, value, &isArray));                                                           \
        if (!isArray)                                                                                                  \
            return false;                                                                                              \
                                                                                                                       \
        try {                                                                                                          \
            *result = ParseArg<std::vector<TYPE>>(env, value);                                                         \
            return true;                                                                                               \
        } catch (...) {                                                                                                \
            return false;                                                                                              \
        }                                                                                                              \
    }

    DEFINE_VECTOR_CONVERSION(int)
    DEFINE_VECTOR_CONVERSION(double)
    DEFINE_VECTOR_CONVERSION(float)
    DEFINE_VECTOR_CONVERSION(std::string)
    DEFINE_VECTOR_CONVERSION(bool)
    DEFINE_VECTOR_CONVERSION(uint32_t)
    DEFINE_VECTOR_CONVERSION(int64_t)


#endif // JSBinding_ArgParse

    static napi_value GetParentConstructor(napi_env env, napi_value exports) {
//        std::string parentName = Derived::Traits::GetParentName();
//        napi_value parent = nullptr;
//        if (!parentName.empty()) {
//            napi_get_named_property(env, exports, parentName.c_str(), &parent);
//        }
//        return parent;
        std::string ns = Derived::Traits::GetNameSpacePath();
        std::string pn = Derived::Traits::GetParentName();
        const std::string parentFullName = ns + (ns.empty() ? "" : ".") + pn;
        if (parentFullName.empty())
            return nullptr;
        std::istringstream iss(parentFullName);
        std::string segment;
        napi_value current = exports;

        while (std::getline(iss, segment, '.')) {
            napi_value next;
            napi_status status = napi_get_named_property(env, current, segment.c_str(), &next);
            if (status != napi_ok || !IsConstructor(env, next)) {
                napi_throw_error(env, nullptr, ("Parent class not found: " + parentFullName).c_str());
                return nullptr;
            }
            current = next;
        }
        return current;
    }

    static NapiWrapper *GetWrapper(napi_env env, napi_value value) {
        NapiWrapper *wrapper = nullptr;
        napi_unwrap(env, value, reinterpret_cast<void **>(&wrapper));
        return wrapper;
    }


public:
#define DEFINE_ASYNC_METHOD(name, methodName)                                                                          \
    { name, nullptr, AsyncMethodWrapper<&DerivedType::methodName>, nullptr, nullptr, nullptr, napi_default, nullptr }

// 定义同步方法的宏
#define DEFINE_METHOD(name, methodName)                                                                                \
    { name, nullptr, MethodWrapper<&DerivedType::methodName>, nullptr, nullptr, nullptr, napi_default, nullptr }

#define DEFINE_STATIC_METHOD(name, methodName)                                                                         \
    { name, nullptr, StaticMethodWrapper<&DerivedType::methodName>, nullptr, nullptr, nullptr, napi_static, nullptr }

#define DEFINE_GETTER(name, methodName)                                                                                \
    { name, nullptr, nullptr, GetterWrapper<&DerivedType::methodName>, nullptr, nullptr, napi_default, nullptr }

#define DEFINE_SETTER(name, methodName)                                                                                \
    { name, nullptr, nullptr, nullptr, SetterWrapper<&DerivedType::methodName>, nullptr, napi_default, nullptr }

#define DEFINE_ACCESSOR(name, getter, setter)                                                                          \
    {                                                                                                                  \
        name, nullptr, nullptr, GetterWrapper<&DerivedType::getter>, SetterWrapper<&DerivedType::setter>, nullptr,     \
            napi_default, nullptr                                                                                      \
    }

#define DEFINE_CONSTANT(name, env, value)                                                                              \
    { name, nullptr, nullptr, nullptr, nullptr, ToNapiValue(env, value), napi_static, nullptr }


    template <typename MethodType>
    static inline napi_property_descriptor DefineMethod(const char *name, MethodType method) {
        static_assert(std::is_member_function_pointer_v<MethodType>, "Method must be a member function pointer");
        return DEFINE_METHOD(name, method);
    }

    template <typename MethodType>
    static inline napi_property_descriptor DefineStaticMethod(const char *name, MethodType method) {
        static_assert(std::is_member_function_pointer_v<MethodType>, "Static Method must be a member function pointer");
        return DEFINE_STATIC_METHOD(name, method);
    }

    template <typename MethodType>
    static inline napi_property_descriptor DefineGetter(const char *name, MethodType method) {
        static_assert(std::is_member_function_pointer_v<MethodType>, "Getter must be a member function pointer");
        return DEFINE_GETTER(name, method);
    }

    template <typename MethodType>
    static inline napi_property_descriptor DefineSetter(const char *name, MethodType method) {
        static_assert(std::is_member_function_pointer_v<MethodType>, "Setter must be a member function pointer");
        return DEFINE_SETTER(name, method);
    }

    template <typename GetterMethodType, typename SetterMethodType>
    static inline napi_property_descriptor DefineAccessor(const char *name, GetterMethodType getter,
                                                          SetterMethodType setter) {
        static_assert(std::is_member_function_pointer_v<GetterMethodType>, "Getter must be a member function pointer");
        static_assert(std::is_member_function_pointer_v<SetterMethodType>, "Setter must be a member function pointer");
        return DEFINE_GETTER_SETTER(name, getter, setter);
    }

    static inline napi_property_descriptor DefineConstant(const char *name, napi_env env, napi_value value) {
        return DEFINE_CONSTANT(name, env, value);
    }

    static napi_value Init(napi_env env, napi_value exports) {
        static_assert(std::is_class_v<typename Derived::Traits>, "Derived must define a nested Traits class");

        // 使用综合检查器
        static_assert(traits_check::ValidateTraits<typename Derived::Traits>::value,
                      "Traits class does not implement all required methods");
        const std::string className = GetClassName();
        std::vector<napi_property_descriptor> methods = GetMethods(env);
        napi_value parent = GetParentConstructor(env, exports);
        napi_value cons = DefineClass(env, className, parent, methods.data(), methods.size(), IsAbstractClass());
        napi_set_named_property(env, exports, className.c_str(), cons);
        return exports;
    }


    // 定义为公共方法，方便其他binding获取实例
    static T *GetNativeInstance(napi_env env, napi_value value) {
        NapiWrapper *wrapper = GetWrapper(env, value);
        return wrapper ? wrapper->GetNative() : nullptr;
    }

    template <typename E = ExtraData>
    static auto GetExtraData(napi_env env, napi_value value) -> std::enable_if_t<!std::is_void<E>::value, E *> {
        if (NapiWrapper *wrapper = GetWrapper(env, value)) {
            return &wrapper->GetExtraData();
        }
    }

    static auto GetNativeShared(napi_env env, napi_value value) {
        if (NapiWrapper *wrapper = GetWrapper(env, value)) {
            return &wrapper->GetShared();
        }
        return nullptr;
    }

    static void RemoveWrapper(T *native) {
        std::lock_guard<std::mutex> lock(nativeToJsMutex_);
        auto it = nativeToJsMap_.find(native);
        if (it != nativeToJsMap_.end()) {
            napi_env env = it->second.first;
            napi_ref ref = it->second.second;

            napi_value jsInstance;
            napi_status status = napi_get_reference_value(env, ref, &jsInstance);
            if (status != napi_ok) {
                nativeToJsMap_.erase(it);
                return;
            }

            void *wrapperData = nullptr;
            status = napi_remove_wrap(env, jsInstance, &wrapperData);
//            if (status == napi_ok && wrapperData != nullptr) {
//                NapiWrapper *wrapper = static_cast<NapiWrapper *>(wrapperData);
//                Derived::Traits::RemoveWrapper(env, wrapper);
//                delete wrapper;
//            }

            napi_delete_reference(env, ref);
            nativeToJsMap_.erase(it);
        }
    }


// getter包装器
    template <auto Method> static napi_value GetterWrapper(napi_env env, napi_callback_info info) {
        try {
            napi_value thisArg;
            NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr));

            // 获取napi包装器
            NapiWrapper *wrapper = GetWrapper(env, thisArg);

            T *native = wrapper->GetNative();
            if (!native) {
                napi_throw_error(env, nullptr, "Failed to get native instance");
                return nullptr;
            }

            if constexpr (!std::is_void_v<ExtraDataType> &&
                          std::is_invocable_v<decltype(Method), T *, ExtraDataType &>) {
                return ToNapiValue(env, Method(native, wrapper->GetExtraData()));
            } else if constexpr (!std::is_void_v<ExtraDataType> &&
                                 std::is_invocable_v<decltype(Method), T *, ExtraDataType &, napi_env>) {
                return Method(native, wrapper->GetExtraData(), env);
            } else if constexpr (std::is_invocable_v<decltype(Method), T *>) {
                return ToNapiValue(env, Method(native));
            } else if constexpr (std::is_invocable_v<decltype(Method), T *, napi_env>) {
                return Method(native, env);
            } else {
                static_assert(true, "Invalid getter method declaration");
                return nullptr;
            }
        } catch (const std::exception &e) {
            napi_throw_error(env, nullptr, e.what());
            return nullptr;
        }
    }

// setter包装器
    template <auto Method> static napi_value SetterWrapper(napi_env env, napi_callback_info info) {
        try {
            size_t argc = 1;
            napi_value value;
            napi_value thisArg;
            NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &value, &thisArg, nullptr));

            if (argc < 1) {
                napi_throw_error(env, nullptr, "Null values are not allowed in the setter");
                return nullptr;
            }

            // 获取napi包装器
            NapiWrapper *wrapper = GetWrapper(env, thisArg);

            T *native = wrapper->GetNative();
            if (!native) {
                napi_throw_error(env, nullptr, "Failed to get native instance");
                return nullptr;
            }

            if constexpr (!std::is_void_v<ExtraDataType> &&
                          std::is_invocable_v<decltype(Method), T *, ExtraDataType &, napi_env, napi_value>) {
                Method(native, wrapper->GetExtraData(), env, value);
            } else if constexpr (std::is_invocable_v<decltype(Method), T *, napi_env, napi_value>) {
                Method(native, env, value);
            } else {
                static_assert(true, "Invalid setter method declaration");
                return nullptr;
            }


            return nullptr;
        } catch (const std::exception &e) {
            napi_throw_error(env, nullptr, e.what());
            return nullptr;
        }
    }

    // 同步方法包装器
    template <auto Method> static napi_value MethodWrapper(napi_env env, napi_callback_info info) {
        try {
            // 获取参数
            size_t argc = 0;
            NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));
            std::vector<napi_value> argv(argc);
            napi_value thisArg;
            NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv.data(), &thisArg, nullptr));

            // 获取napi包装器
            NapiWrapper *wrapper = GetWrapper(env, thisArg);

            JSCallbackInfo info{.env = env, .argv = argv, .thisArgs = thisArg, .wrapper = wrapper};


            if constexpr (std::is_invocable_v<decltype(Method), const JSCallbackInfo &>) {
                if constexpr (std::is_same_v<std::invoke_result_t<decltype(Method), const JSCallbackInfo &>,
                                             napi_value>) {
                    return Method(info);
                } else {
                    return ToNapiValue(env, Method(info));
                }
            }

            T *native = wrapper->GetNative();
            if (!native) {
                napi_throw_error(env, nullptr, "Failed to get native instance");
                return nullptr;
            }

            if constexpr (!std::is_void_v<ExtraDataType> &&
                          std::is_invocable_v<decltype(Method), T *, ExtraDataType &, napi_env, size_t, napi_value *>) {
                // 方法签名：RetType(T*， ExtraData&, napi_env, size_t,napi_value*)
                return Method(native, wrapper->GetExtraData(), env, argc, argv.data());
            } else if constexpr (std::is_invocable_v<decltype(Method), T *, napi_env, size_t, napi_value *>) {
                // 方法签名: RetType(T*, napi_env, size_t, napi_value*)
                return Method(native, env, argc, argv.data());
            } else if constexpr (!std::is_void_v<ExtraDataType> &&
                                 std::is_invocable_v<decltype(Method), ExtraDataType &, T *, napi_env>) {
                // 方法签名: RetType(T*, ExtraData&,napi_env)
                return Method(native, wrapper->GetExtraData(), env);
            } else if constexpr (std::is_invocable_v<decltype(Method), T *, napi_env>) {
                // 方法签名: RetType(T*, napi_env)
                return Method(native, env);
            } else if constexpr (!std::is_void_v<ExtraDataType> &&
                                 std::is_invocable_v<decltype(Method), T *, ExtraDataType &>) {
                // 方法签名: RetType(T*, ExtraData&);
                return Method(native, wrapper->GetExtraData());
            } else if constexpr (std::is_invocable_v<decltype(Method), T *>) {
                // 方法签名: RetType(T*)
                if constexpr (std::is_void_v<std::invoke_result_t<decltype(Method), T *>>) {
                    Method(native);
                    return nullptr;
                } else {
                    auto result = Method(native);
                    return ToNapiValue(env, result);
                }
            }

            napi_throw_error(env, nullptr, "Invalid method signature");
            return nullptr;
        } catch (const std::exception &e) {
            napi_throw_error(env, nullptr, e.what());
            return nullptr;
        }
    }

    template <auto Method> static napi_value StaticMethodWrapper(napi_env env, napi_callback_info info) {
        size_t argc = 0;
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));
        std::vector<napi_value> argv(argc);
        NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv.data(), nullptr, nullptr));
        // 懒得判断了
        Method(env, argc, argv.data());
    }

private:
    template <typename ObjectType> static napi_value ConvertToJSObject(napi_env env, const ObjectType &obj) {
        napi_value jsObj;
        NAPI_CALL(env, napi_create_object(env, &jsObj))
        if constexpr (is_exportable_struct_v<ObjectType>) {
            ExportStructFields(env, jsObj, obj);
        } else if constexpr (has_iterator_v<ObjectType>) {
            for (const auto &[key, val] : obj) {
                NAPI_CALL(env, napi_set_property(env, jsObj, ToNapiValue(env, key), ConvertToJSObject(env, val)))
            }
        }
        return jsObj;
    }

    template <typename ObjectType>
    static void ExportStructFields(napi_env env, napi_value jsObj, const ObjectType &structInst) {
        static_assert(is_exportable_struct_v<ObjectType>, "Type must declare __js_binding_fields");
        for (const auto &[name, memberPtr] : ObjectType::__js_binding_fields) {
            auto &&value = structInst.*memberPtr;
            NAPI_CALL(env, napi_set_named_property(env, jsObj, name, ConvertToJSObject(env, value)))
        }
    }

    static bool IsConstructor(napi_env env, napi_value value) {
        napi_valuetype type;
        napi_typeof(env, value, &type);
        if (type != napi_function)
            return false;

        napi_value prototype;
        napi_get_named_property(env, value, "prototype", &prototype);

        napi_valuetype prototypeType;
        napi_typeof(env, prototype, &prototypeType);

        return prototypeType == napi_object;
    }


    template <typename ObjectType, typename = void> struct has_iterator : std::false_type {};

    template <typename ObjectType>
    struct has_iterator<ObjectType, std::void_t<decltype(std::declval<ObjectType>().begin()),
                                                decltype(std::declval<ObjectType>().end())>> : std::true_type {};
    template <typename ObjectType> static constexpr bool has_iterator_v = has_iterator<ObjectType>::value;

    template <typename ObjectType, typename = void> struct is_exportable_struct : std::false_type {};

    template <typename ObjectType>
    struct is_exportable_struct<ObjectType, std::void_t<decltype(ObjectType::__js_binding_fields)>> : std::true_type {};

    template <typename ObjectType> static constexpr bool is_exportable_struct_v = is_exportable_struct<T>::value;

    inline static std::unordered_map<T *, std::pair<napi_env, napi_ref>> nativeToJsMap_;
    inline static std::mutex nativeToJsMutex_;
};

#define JS_BINDING_STRUCT(type, ...)                                                                                   \
    static constexpr auto__js_binding_fields = std::make_tuple(__VA_ARGS__);                                           \
    using __js_binding_type = type

// ... existing code ...

// 添加导出系统宏
#define DECLARE_ROOT_START(ns)                                                                                         \
    struct __##ns##_##ExportRegistry {                                                                                 \
        static void Init(napi_env env, napi_value exports) {                                                           \
            std::vector<std::function<void(napi_env, napi_value)>> __exportStack;                                      \
            std::vector<std::string> __nsStack;                                                                        \
            napi_value __currentTarget = exports;                                                                      \
            std::vector<napi_value> __targetStack;

#define DECLARE_NAMESPACE_START(name)                                                                                  \
    __nsStack.push_back(#name);                                                                                        \
    napi_value __ns_##name;                                                                                            \
    napi_create_object(env, &__ns_##name);                                                                             \
    napi_set_named_property(env, __currentTarget, #name, __ns_##name);                                                 \
    __targetStack.push_back(__currentTarget);                                                                          \
    __currentTarget = __ns_##name;

#define DECLARE_NAMESPACE_END                                                                                          \
    __nsStack.pop_back();                                                                                              \
    __currentTarget = __targetStack.back();                                                                            \
    __targetStack.pop_back();

#define DECLARE_CLASS(ClassName)                                                                                       \
    __exportStack.emplace_back([&](napi_env env, napi_value target) {                                                  \
        const std::string nsPath = GetCurrentNamespace(__nsStack);                                                     \
        ClassName::Traits::SetNameSpacePath(nsPath);                                                                   \
        ClassName::ExportTopLevel(env, target);                                                                        \
    });

#define DECLARE_CONSTANT(key, value)                                                                                   \
    __exportStack.emplace_back(                                                                                        \
        [=](napi_env env, napi_value target) { JSBinding<>::ExportConstant(env, target, key, value); });

#define DECLARE_OBJECT(key, obj)                                                                                       \
    __exportStack.emplace_back(                                                                                        \
        [=](napi_env env, napi_value target) { JSBinding<>::ExportObject(env, target, key, obj); });

#define DECLARE_ROOT_END                                                                                               \
    for (auto &exporter : __exportStack) {                                                                             \
        exporter(env, __currentTarget);                                                                                \
    }                                                                                                                  \
    }                                                                                                                  \
    }                                                                                                                  \
    ;

static std::string GetCurrentNamespace(const std::vector<std::string> &stack) {
    if (stack.empty())
        return "";
    std::string path;
    for (const auto &ns : stack) {
        path += ns + ".";
    }
    return path.substr(0, path.size() - 1);
}

using JSBindingTool = JSBinding<void, void>;

#endif // JEMOC_STREAM_TEST_JSBINDING_H
