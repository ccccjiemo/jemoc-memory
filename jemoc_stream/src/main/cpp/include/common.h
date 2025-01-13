//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_UTILS_H
#define JEMOC_STREAM_TEST_UTILS_H

#include <napi/native_api.h>
#include <string>
#include <ios>

typedef unsigned char byte;
class IStream;

#define NAPI_CALL(env, func) NAPI_CALL_BASE(env, func, __LINE__)

#define NAPI_CALL_BASE(env, func, line)                                                                                \
    if (napi_ok != func) {                                                                                             \
        napi_throw_error(env, "NAPI_CALL_ERROR", #func);                                            \
    }

// #define NAPI_CALL(env, call)                                                                                           \
//     do {                                                                                                               \
//         if (call != napi_ok) {                                                                                         \
//             const napi_extended_error_info *error_info;                                                                \
//             napi_get_last_error_info(env, &error_info);                                                                \
//             const char *error_message = error_info->error_message;                                                     \
//             char error[100] = {'\0'};                                                                                  \
//             printf(error, "NAPI Error at line %d: %s; call %s.\n", __LINE__, error_message, #call);                   \
//             napi_throw_error(env, "NAPI_CALL", error);                                                                 \
//         }                                                                                                              \
//     } while (0);

#define DEFINE_NAPI_FUNCTION(name, func, getter, setter, data)                                                         \
    { name, nullptr, func, getter, setter, nullptr, napi_default, data }


#define GET_OBJ(obj, name, func, result)                                                                               \
    napi_get_named_property(env, obj, name, &value);                                                                   \
    napi_typeof(env, value, &type);                                                                                    \
    if (type != napi_undefined) {                                                                                      \
        func(env, value, &result);                                                                                     \
    }

// unsafe
static void *offset_pointer(void *target, long offset) {
    byte *buffer = static_cast<byte *>(target) + offset;
    if (buffer == nullptr)
        throw std::ios_base::failure("null pointer");
    return buffer;
}

static IStream *getStream(napi_env env, napi_value value) {
    void *strm = nullptr;
    napi_unwrap(env, value, &strm);
    return static_cast<IStream *>(strm);
}

static long getLong(napi_env env, napi_value value) {
    long result = 0;
    napi_get_value_int64(env, value, &result);
    return result;
}

static int getInt(napi_env env, napi_value value) {
    int result = 0;
    napi_get_value_int32(env, value, &result);
    return result;
}

static void getBuffer(napi_env env, napi_value value, void **data, size_t *length) {
    bool isTargetBuffer = false;
    napi_is_arraybuffer(env, value, &isTargetBuffer);
    if (isTargetBuffer) {
        napi_get_arraybuffer_info(env, value, data, length);
        return;
    }
    napi_is_typedarray(env, value, &isTargetBuffer);
    if (isTargetBuffer) {
        napi_get_typedarray_info(env, value, nullptr, length, data, nullptr, nullptr);
        return;
    }
    *data = nullptr;
    *length = 0;
}

/**
 * 获取napi的offset参数，它可能是个undefined
 * @param env
 * @param value
 * @return
 */
static long getOffset(napi_env env, napi_value value, long bufferSize) {
    napi_valuetype type;
    napi_typeof(env, value, &type);
    if (type == napi_undefined)
        return 0;
    long result = getLong(env, value);
    if (result < 0 || result > bufferSize) {
        napi_throw_range_error(env, "IStream", "get offset is out of range");
    }
    return result;
}

/**
 * 获取napi的count参数，它可能是个undefifned
 * @param env
 * @param value
 * @param bufferSize arraybuffer的长度
 * @param offset arraybuffer的偏移
 * @return
 */
static long getCount(napi_env env, napi_value value, long bufferSize, long offset) {
    napi_valuetype type;
    napi_typeof(env, value, &type);
    if (type == napi_undefined)
        return bufferSize - offset;
    long result = getLong(env, value);
    if (result > bufferSize - offset) {
        napi_throw_range_error(env, "IStream", "get count is out of range");
    }
    return result;
}

static const std::string getString(napi_env env, napi_value value) {
    napi_valuetype type;
    napi_typeof(env, value, &type);
    if (type != napi_string)
        return "";
    size_t size = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &size);
    if (size == 0)
        return "";

    char *buffer = new char[size + 1];
    napi_get_value_string_utf8(env, value, buffer, size + 1, &size);
    std::string result(buffer);
    delete[] buffer;
    return result;
}


#endif // JEMOC_STREAM_TEST_UTILS_H