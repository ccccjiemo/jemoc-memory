//
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "BufferPool.h"

namespace jemoc_stream {
ThreadLocalBufferPool::ThreadLocalBufferPool(size_t bufferSize) : bufferSize_(bufferSize) {}

shared_ptr<uint8_t> ThreadLocalBufferPool::acquire(size_t size) {
    if (size > bufferSize_) {
        return nullptr;
    }

    thread_local std::list<std::shared_ptr<uint8_t>> localPool;
    thread_local std::shared_ptr<uint8_t> currentBuffer;

    if (!currentBuffer || localPool.empty()) {
        currentBuffer = std::shared_ptr<uint8_t>(new uint8_t[bufferSize_], [](uint8_t *p) { delete[] p; });
        localPool.push_back(currentBuffer);
    }

    auto buffer = localPool.front();
    localPool.pop_front();
    updateStats(true);
    return buffer;
}

void ThreadLocalBufferPool::release(shared_ptr<uint8_t> buffer) {
    thread_local std::list<std::shared_ptr<uint8_t>> localPool;
    localPool.push_back(buffer);
    updateStats(false);
}
}