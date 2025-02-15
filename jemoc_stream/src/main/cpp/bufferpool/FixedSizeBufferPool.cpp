//
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "BufferPool.h"

namespace jemoc_stream {


FixedSizeBufferPool::FixedSizeBufferPool(size_t bufferSize, size_t maxBuffers)
    : bufferSize_(bufferSize), maxBuffers_(maxBuffers) {}

shared_ptr<uint8_t> FixedSizeBufferPool::acquire(size_t size) {
    if (size > bufferSize_) {
        return nullptr;
    }

    lock_guard<mutex> lock(mutex_);

    if (!freeList_.empty()) {
        auto buffer = freeList_.front();
        freeList_.pop_front();
        usedList_.push_back(buffer);
        updateStats(true);
        return buffer;
    }

    if (usedList_.size() + freeList_.size() < maxBuffers_) {
        auto buffer = std::shared_ptr<uint8_t>(new uint8_t[bufferSize_], [](uint8_t *p) { delete[] p; });
        usedList_.push_back(buffer);
        updateStats(true);
        return buffer;
    }

    return nullptr;
}

void FixedSizeBufferPool::release(shared_ptr<uint8_t> buffer) {
    lock_guard<mutex> lock(mutex_);

    auto it = std::find(usedList_.begin(), usedList_.end(), buffer);
    if (it != usedList_.end()) {
        usedList_.erase(it);
        freeList_.push_back(buffer);
        updateStats(false);
    }
}
}