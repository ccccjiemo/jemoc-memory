//
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <list>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <napi/native_api.h>
#include "common.h"


namespace jemoc_stream {

// 添加std命名空间引用
using std::shared_ptr;
using std::unique_ptr;
using std::make_unique;
using std::lock_guard;
using std::mutex;
using std::list;
using std::unordered_map;
using std::size_t;
using std::vector;
using std::atomic;
using std::thread;


/**
 * @brief Memory buffer pool base class providing common statistics tracking
 *
 * Implementations:
 * - LruBufferPool: Least Recently Used eviction policy
 * - FixedSizeBufferPool: Pre-allocated fixed size buffers
 *
 * Usage example:
 * @code
 * // Create pool with 1MB LRU cache
 * auto lruPool = BufferPool::create(BufferPool::Type::LRU, 1024*1024);
 *
 * // Create fixed size pool with 100 buffers of 4KB each
 * auto fixedPool = BufferPool::create(BufferPool::Type::FIXED_SIZE, 4096, 100);
 *
 * // Acquire buffer
 * auto buf = pool->acquire(1024);
 *
 * // Release buffer
 * pool->release(buf);
 * @endcode
 */

class BufferPool;

struct BufferData {
    BufferPool *pool = nullptr;
    shared_ptr<uint8_t> buffer = nullptr;
};

class BufferPool {
public:
    enum class Type { LRU, FIXED_SIZE, DYNAMIC, RING, THREAD_LOCAL };
    static const size_t Alignment = 64; // 根据ARM要求可调整

    static shared_ptr<uint8_t> allocateAlignedBuffer(size_t size);

    virtual ~BufferPool() noexcept = default;
    virtual shared_ptr<uint8_t> acquire(size_t size) = 0;
    virtual void release(shared_ptr<uint8_t> buffer) = 0;
    void stats(size_t &total, size_t &used) const;

public:
    static void Export(napi_env env, napi_value exports);
    static napi_value JSAcquire(napi_env env, napi_callback_info info);
    static napi_value JSRelease(napi_env env, napi_callback_info info);
    static napi_value JSGetStats(napi_env env, napi_callback_info info);
    static napi_value JSUpdateStats(napi_env env, napi_callback_info info);
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static std::string AbstractClassName;
    static napi_value cons;
    static void Extends(napi_env env, napi_value cons);

protected:
    mutable mutex statsMutex_;
    size_t totalBuffers_ = 0;
    size_t usedBuffers_ = 0;

    unordered_map<void *, std::shared_ptr<byte>> acquiredList;
    virtual void updateStats(bool acquiring);
};

class LruBufferPool : public BufferPool {
protected:
    void updateStats(bool acquiring) override;
public:
    // LRU buffer pool with a maximum size in bytes
    explicit LruBufferPool(size_t maxSize);
    ~LruBufferPool() override;

    shared_ptr<uint8_t> acquire(size_t size) override;
    void release(shared_ptr<uint8_t> buffer) override;

public:
    static std::string ClassName;
    static void Export(napi_env env, napi_value exports);
    static napi_value JSConstructor(napi_env env, napi_callback_info info);

private:
    struct BufferEntry {
        shared_ptr<uint8_t> buffer;
        size_t size;
    };

    mutable mutex mutex_;
    size_t maxSize_;
    size_t currentSize_ = 0;
    list<shared_ptr<uint8_t>> lruList_;
    unordered_map<uint8_t *, BufferEntry> bufferMap_;
};

class DynamicBufferPool : public BufferPool {
public:
    explicit DynamicBufferPool(size_t initSize);

    shared_ptr<uint8_t> acquire(size_t size) override;
    void release(shared_ptr<uint8_t> buffer) override;

public:
    static std::string ClassName;
    static napi_ref cons;

private:
    struct Chunk {
        shared_ptr<uint8_t> memory;
        size_t size;
        bool used;
    };

    mutable mutex mutex_;
    size_t chunkSize_;
    vector<Chunk> chunks_;
    unordered_map<uint8_t *, Chunk *> chunkMap_;
};

class RingBufferPool : public BufferPool {
public:
    RingBufferPool(size_t bufferSize, size_t numBuffers);

    shared_ptr<uint8_t> acquire(size_t size) override;
    void release(shared_ptr<uint8_t> buffer) override;

private:
    struct RingSlot {
        shared_ptr<uint8_t> buffer;
        bool available = true;
    };

    mutable mutex mutex_;
    size_t bufferSize_;
    vector<RingSlot> ring_;
    size_t head_ = 0;
    size_t tail_ = 0;
    atomic<size_t> freeSlots_;
};

class ThreadLocalBufferPool : public BufferPool {
public:
    explicit ThreadLocalBufferPool(size_t bufferSize);

    shared_ptr<uint8_t> acquire(size_t size) override;
    void release(shared_ptr<uint8_t> buffer) override;

private:
    struct ThreadLocalData {
        list<shared_ptr<uint8_t>> freeBuffers;
        list<shared_ptr<uint8_t>> usedBuffers;
    };

    size_t bufferSize_;
    mutex initMutex_;
    unordered_map<thread::id, unique_ptr<ThreadLocalData>> threadData_;
};

class FixedSizeBufferPool : public BufferPool {
public:
    FixedSizeBufferPool(size_t bufferSize, size_t maxBuffers);

    shared_ptr<uint8_t> acquire(size_t size) override;
    void release(shared_ptr<uint8_t> buffer) override;

private:
    mutable mutex mutex_;
    size_t bufferSize_;
    size_t maxBuffers_;
    list<shared_ptr<uint8_t>> freeList_;
    list<shared_ptr<uint8_t>> usedList_;
};

} // namespace jemoc_stream
