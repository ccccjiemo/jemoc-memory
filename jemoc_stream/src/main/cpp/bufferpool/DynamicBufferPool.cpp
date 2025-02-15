//
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "BufferPool.h"

namespace jemoc_stream {


DynamicBufferPool::DynamicBufferPool(size_t initSize) : chunkSize_(initSize) {}

shared_ptr<uint8_t> DynamicBufferPool::acquire(size_t size) {
    lock_guard<mutex> lock(mutex_);
    
    // Try to find free chunk
    auto it = find_if(chunks_.begin(), chunks_.end(),
        [size](const Chunk& chunk) {
            return !chunk.used && chunk.size >= size;
        });
    
    if (it != chunks_.end()) {
        it->used = true;
        chunkMap_[it->memory.get()] = &(*it);
        updateStats(true);
        return it->memory;
    }
    
    // Allocate new chunk
    size_t allocSize = std::max(size, chunkSize_);
    auto memory = std::shared_ptr<uint8_t>(
        new uint8_t[allocSize],
        [](uint8_t* p) { delete[] p; }
    );
    
    chunks_.push_back({memory, allocSize, true});
    chunkMap_[memory.get()] = &chunks_.back();
    updateStats(true);
    return memory;
}

void DynamicBufferPool::release(shared_ptr<uint8_t> buffer) {
    lock_guard<mutex> lock(mutex_);
    auto it = chunkMap_.find(buffer.get());
    if (it != chunkMap_.end()) {
        it->second->used = false;
        chunkMap_.erase(it);
        updateStats(false);
    }
}
}