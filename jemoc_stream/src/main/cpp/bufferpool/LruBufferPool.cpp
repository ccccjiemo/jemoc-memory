//
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#include "BufferPool.h"

namespace jemoc_stream {

std::string LruBufferPool::ClassName = "LruBufferPool";

LruBufferPool::LruBufferPool(size_t maxSize) : maxSize_(maxSize) {}

LruBufferPool::~LruBufferPool() = default;

void LruBufferPool::updateStats(bool acquiring) {
    lock_guard<mutex> lock(statsMutex_);
    if (acquiring) {
        usedBuffers_++;
        totalBuffers_++;
    } else {
        usedBuffers_--;
    }
}

shared_ptr<uint8_t> LruBufferPool::acquire(size_t size) {
    lock_guard<mutex> lock(mutex_);

    // Try to find existing buffer
    auto it =
        find_if(bufferMap_.begin(), bufferMap_.end(), [size](const auto &entry) { return entry.second.size >= size; });

    if (it != bufferMap_.end()) {
        // Move to front of LRU list
        lruList_.remove(it->second.buffer);
        lruList_.push_front(it->second.buffer);
        return it->second.buffer;
    }

    // Allocate new buffer
    auto buffer = std::shared_ptr<uint8_t>(static_cast<uint8_t *>(malloc(size)), [](uint8_t *ptr) { free(ptr); });

    // Add to buffer map
    bufferMap_[buffer.get()] = {buffer, size};
    currentSize_ += size;

    // Maintain max size
    while (currentSize_ > maxSize_ && !lruList_.empty()) {
        auto lru = lruList_.back();
        auto &entry = bufferMap_[lru.get()];
        currentSize_ -= entry.size;
        bufferMap_.erase(lru.get());
        lruList_.pop_back();
    }

    lruList_.push_front(buffer);
    return buffer;
}

void LruBufferPool::release(shared_ptr<uint8_t> buffer) {
    lock_guard<mutex> lock(mutex_);
    // Update LRU position
    lruList_.remove(buffer);
    lruList_.push_front(buffer);
}

napi_value LruBufferPool::JSConstructor(napi_env env, napi_callback_info info) {
    napi_value _this = nullptr;
    size_t argc = 1;
    napi_value argv[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &_this, nullptr))
    long size = 0;
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    if (type != napi_number) {
        napi_throw_type_error(env, ClassName.c_str(), "size must be a number");
        return nullptr;
    }
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &size))
    if (size <= 0) {
        napi_throw_range_error(env, ClassName.c_str(), "size must >= 0");
        return nullptr;
    }
    LruBufferPool *pool = new LruBufferPool(size);
    NAPI_CALL(env, napi_wrap(
                       env, _this, pool, [](napi_env env, void *data, void *hint) {}, nullptr, nullptr))
    return _this;
}

void LruBufferPool::Export(napi_env env, napi_value exports) {
    napi_value js_cons;
    NAPI_CALL(env,
              napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, 0, nullptr, &js_cons))
    Extends(env, js_cons);
    NAPI_CALL(env, napi_set_named_property(env, exports, ClassName.c_str(), js_cons));
}

}
