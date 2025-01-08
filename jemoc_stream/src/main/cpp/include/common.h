//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_UTILS_H
#define JEMOC_STREAM_TEST_UTILS_H

#include <napi/native_api.h>
#include <IStream.h>

#define DEFINE_NAPI_FUNCTION(name, func, getter, setter)                                                               \
    { name, nullptr, func, getter, setter, nullptr, napi_default, nullptr }



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


#endif // JEMOC_STREAM_TEST_UTILS_H