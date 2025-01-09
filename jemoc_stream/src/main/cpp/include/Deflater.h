//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_DEFLATER_H
#define JEMOC_STREAM_TEST_DEFLATER_H
#include "zlib-ng.h"
#include <cstddef>
#include <mutex>

#define Min_WINDOW_BITS -15
#define Max_WINDOW_BITS 31

class Deflater {
public:
    Deflater(int windowBits, int level, int strategy);
    ~Deflater();
    
    void setInput(void *buffer, size_t count);
    bool needInput() const;
    bool finish(void* buffer, size_t count, size_t* bytesRead);
    long getDeflateOutput(void* buffer, size_t count);
    bool flush(void* buffer, size_t count, size_t * bytesRead);
    
private:
    int readDeflateOutput(void* buffer, size_t count, int flushCode, size_t* bytesRead);
    int deflate(int flushCode);
    
private:
    int m_windowBits;
    int m_level;
    int m_strategy;
    zng_stream *zStream;
    std::mutex mtx;
};

#endif // JEMOC_STREAM_TEST_DEFLATER_H
