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
#include <sys/types.h>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <hilog/log.h>



typedef unsigned char byte;
class IStream;

#define LOG_LIB_DOMAIN 0x3000
#define LOG_LIB_TAG "@jemoc/stream"

#define NAPI_CALL(env, func) NAPI_CALL_BASE(env, func, __LINE__)

#define NAPI_CALL_BASE(env, func, line)                                                                                \
    if (napi_ok != func) {                                                                                             \
        napi_throw_error(env, "NAPI_CALL_ERROR", #func);                                                               \
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

static std::shared_ptr<IStream> *getStream(napi_env env, napi_value value) {
    void *strm = nullptr;
    napi_unwrap(env, value, &strm);
    return static_cast<std::shared_ptr<IStream>*>(strm);
}

static long getLong(napi_env env, napi_value value) {
    long result = 0;
    napi_get_value_int64(env, value, &result);
    return result;
}

static int getInt(napi_env env, napi_value value) {
    int result = 0;
    napi_valuetype isNum;
    NAPI_CALL(env, napi_typeof(env, value, &isNum))
    if (isNum == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, value, &result))
    }
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


static double dostime_to_unix_timestamp(uint32_t dostime) {
    uint16_t dos_date = static_cast<uint16_t>(dostime >> 16);
    uint16_t dos_time = static_cast<uint16_t>(dostime & 0xFFFF);

    struct tm t = {0};                         // 初始化为0
    t.tm_sec = (dos_time & 0x1F) * 2;          // 5 bits for seconds (0-29, representing 0-58 seconds)
    t.tm_min = (dos_time >> 5) & 0x3F;         // 6 bits for minutes (0-59)
    t.tm_hour = (dos_time >> 11) & 0x1F;       // 5 bits for hours (0-23)
    t.tm_mday = dos_date & 0x1F;               // 5 bits for day of month (1-31)
    t.tm_mon = ((dos_date >> 5) & 0x0F) - 1;   // 4 bits for month (1-12, 0-11 for tm_mon)
    t.tm_year = ((dos_date >> 9) & 0x7F) + 80; // 7 bits for year (from 1980, tm_year is years since 1900)
    t.tm_isdst = -1;                           // Not considering daylight saving time

    return static_cast<double>(mktime(&t)) * 1000.0; // 转换为毫秒
}

static uint32_t unix_timestamp_to_dostime(double unix_timestamp) {
    time_t seconds = static_cast<time_t>(unix_timestamp / 1000.0);
    struct tm *t = localtime(&seconds);

    uint16_t dos_date = (t->tm_mday & 0x1F) | ((t->tm_mon + 1) << 5 & 0x1E0) | ((t->tm_year - 80) << 9 & 0xFE00);
    uint16_t dos_time = (t->tm_sec / 2 & 0x1F) | (t->tm_min << 5 & 0x7E0) | (t->tm_hour << 11 & 0xF800);

    return (dos_date << 16) | dos_time;
}

static uint32_t get_current_dostime() {
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);

    uint16_t dos_date = (t->tm_mday & 0x1F) | ((t->tm_mon + 1) << 5 & 0x1E0) | ((t->tm_year - 80) << 9 & 0xFE00);
    uint16_t dos_time = (t->tm_sec / 2 & 0x1F) | (t->tm_min << 5 & 0x7E0) | (t->tm_hour << 11 & 0xF800);

    return (dos_date << 16) | dos_time;
}

static double get_current_unix_timestamp() {
    using namespace std::chrono;
    milliseconds ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return static_cast<double>(ms.count());
}

template <typename K, typename V> class BiMap {
private:
    std::unordered_map<K, V> leftMap;
    std::unordered_map<V, K> rightMap;

public:
    // 插入或更新键值对
    void set(const K &key, const V &value) {
        // 如果 key 已存在，先移除其旧的反向映射
        auto itKey = leftMap.find(key);
        if (itKey != leftMap.end()) {
            rightMap.erase(itKey->second);
        }
        // 如果 value 已存在，先移除其旧的正向映射
        auto itValue = rightMap.find(value);
        if (itValue != rightMap.end()) {
            leftMap.erase(itValue->second);
        }
        leftMap[key] = value;
        rightMap[value] = key;
    }

    // 通过键获取值，返回是否找到
    bool getValue(const K &key, V &value) const {
        auto it = leftMap.find(key);
        if (it != leftMap.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    // 通过值获取键，返回是否找到
    bool getKey(const V &value, K &key) const {
        auto it = rightMap.find(value);
        if (it != rightMap.end()) {
            key = it->second;
            return true;
        }
        return false;
    }

    // 根据键删除映射
    bool deleteByKey(const K &key) {
        auto it = leftMap.find(key);
        if (it == leftMap.end())
            return false;
        rightMap.erase(it->second);
        leftMap.erase(it);
        return true;
    }

    // 根据值删除映射
    bool deleteByValue(const V &value) {
        auto it = rightMap.find(value);
        if (it == rightMap.end())
            return false;
        leftMap.erase(it->second);
        rightMap.erase(it);
        return true;
    }

    // 清空所有映射
    void clear() {
        leftMap.clear();
        rightMap.clear();
    }
};

#endif // JEMOC_STREAM_TEST_UTILS_H