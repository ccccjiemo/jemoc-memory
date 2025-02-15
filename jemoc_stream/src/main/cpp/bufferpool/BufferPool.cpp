//
// Created on 2025/2/13.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "BufferPool.h"
#include <unistd.h>


#define GET_JS_INFO_WITH_BUFFER_POOL(count)                                                                            \
    napi_value _this = nullptr;                                                                                        \
    size_t argc = count;                                                                                               \
    napi_value argv[count];                                                                                            \
    void *className;                                                                                                   \
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &_this, &className))                                       \
    const char *tagName = static_cast<char *>(className);                                                              \
    void *pool_ = nullptr;                                                                                             \
    NAPI_CALL(env, napi_unwrap(env, _this, &pool_))                                                                    \
    if (pool_ == nullptr) {                                                                                            \
        napi_throw_type_error(env, tagName, "bufferpool is nullptr or released");                                      \
        return nullptr;                                                                                                \
    }                                                                                                                  \
    BufferPool *pool = static_cast<BufferPool *>(pool_);


namespace jemoc_stream {
std::string BufferPool::AbstractClassName = "BufferPool";
napi_value BufferPool::cons = nullptr;

napi_value BufferPool::JSAcquire(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITH_BUFFER_POOL(1)
    long size = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &size))
    if (size <= 0) {
        napi_throw_range_error(env, tagName, "acquire buffer size must >= 0");
        return nullptr;
    }
    try {
        shared_ptr<u_int8_t> buffer = pool->acquire(size);
        napi_value result = nullptr;
        napi_create_external_arraybuffer(
            env, buffer.get(), size,
            [](napi_env env, void *data, void *hint) {
                try {
                    BufferPool *pool = reinterpret_cast<BufferPool *>(hint);
                    auto it = pool->acquiredList.find(data);
                    if (it != pool->acquiredList.end()) {
                        pool->release(it->second);
                        pool->acquiredList.erase(it);
                    }
                } catch (std::exception &e) {
                }
            },
            pool, &result);
        pool->acquiredList[buffer.get()] = buffer;
        return result;

    } catch (const std::exception &e) {
        napi_throw_error(env, tagName, e.what());
        return nullptr;
    }
}

napi_value BufferPool::JSRelease(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITH_BUFFER_POOL(1)
    bool checked = false;
    NAPI_CALL(env, napi_is_arraybuffer(env, argv[0], &checked))
    if (!checked) {
        napi_throw_type_error(env, tagName, "args is not arraybuffer");
        return nullptr;
    }
    void *data = nullptr;
    size_t size = 0;
    NAPI_CALL(env, napi_get_arraybuffer_info(env, argv[0], &data, &size));

    auto it = pool->acquiredList.find(data);
    if (it == pool->acquiredList.end()) {
        return nullptr;
    }

    pool->release(it->second);
    pool->acquiredList.erase(it);
    NAPI_CALL(env, napi_detach_arraybuffer(env, argv[0]))
    return nullptr;
}

napi_value BufferPool::JSGetStats(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITH_BUFFER_POOL(0)
    size_t total = 0;
    size_t used = 0;
    pool->stats(total, used);
    napi_value result = nullptr;
    napi_value js_total = nullptr;
    napi_value js_used = nullptr;
    NAPI_CALL(env, napi_create_int64(env, total, &js_total))
    NAPI_CALL(env, napi_create_int64(env, used, &js_used))
    NAPI_CALL(env, napi_create_object(env, &result))
    NAPI_CALL(env, napi_set_named_property(env, result, "total", js_total))
    NAPI_CALL(env, napi_set_named_property(env, result, "used", js_used))
    return result;
}

napi_value BufferPool::JSUpdateStats(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITH_BUFFER_POOL(0)
    bool result = false;
    napi_get_value_bool(env, argv[0], &result);
    pool->updateStats(result);
    return nullptr;
}

void BufferPool::Export(napi_env env, napi_value exports) {
    void *tagName = (void *)BufferPool::AbstractClassName.c_str();
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_FUNCTION("acquire", BufferPool::JSAcquire, nullptr, nullptr, tagName),
        DEFINE_NAPI_FUNCTION("release", BufferPool::JSRelease, nullptr, nullptr, tagName),
        DEFINE_NAPI_FUNCTION("stats", nullptr, BufferPool::JSGetStats, nullptr, tagName),
        DEFINE_NAPI_FUNCTION("updateStats", JSUpdateStats, nullptr, nullptr, tagName),
    };
    NAPI_CALL(env, napi_define_class(env, BufferPool::AbstractClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor,
                                     nullptr, sizeof(desc) / sizeof(desc[0]), desc, &cons))
    NAPI_CALL(env, napi_set_named_property(env, exports, BufferPool::AbstractClassName.c_str(), cons))
}


void BufferPool::updateStats(bool acquiring) {
    lock_guard<mutex> lock(statsMutex_);
    if (acquiring) {
        usedBuffers_++;
        totalBuffers_++;
    } else {
        usedBuffers_--;
    }
}

void BufferPool::stats(size_t &total, size_t &used) const {
    lock_guard<mutex> lock(statsMutex_);
    total = totalBuffers_ * sizeof(uint8_t);
    used = usedBuffers_ * sizeof(uint8_t);
}

shared_ptr<uint8_t> BufferPool::allocateAlignedBuffer(size_t size) {
    uint8_t *ptr = new uint8_t[size];
    auto deleter = [](uint8_t *p) { delete[] p; };
    return shared_ptr<uint8_t>(ptr, deleter);
}

napi_value BufferPool::JSConstructor(napi_env env, napi_callback_info info) { return nullptr; }

void BufferPool::Extends(napi_env env, napi_value constructor) {
    napi_value baseCon = nullptr;
    napi_value extendCon = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, cons, "prototype", &baseCon))
    NAPI_CALL(env, napi_get_named_property(env, constructor, "prototype", &extendCon))
    NAPI_CALL(env, napi_set_named_property(env, extendCon, "__proto__", baseCon))
}


}