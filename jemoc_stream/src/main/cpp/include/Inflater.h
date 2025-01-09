//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_INFLATER_H
#define JEMOC_STREAM_TEST_INFLATER_H
#include "zlib-ng.h"

#define GZIP_Header_ID1 31
#define GZIP_Header_ID2 139

class Inflater {
public:
    Inflater(int windowBits, long uncompressedSize = -1);
    ~Inflater();

    bool isFinished() const;
    long inflate(void *buffer, size_t count);
    bool needInput() const;
    bool isGzipStream() const;
    void setInput(void* buffer, size_t count);

private:
    long readOutput(void *buffer, size_t count);
    long readInflateOutput(void *buffer, size_t count, int flushCode, int *state);
    int inflate_(int flushCode);
    bool resetStreamForLeftoverInput();

private:
    int m_windowBits;
    bool m_finished;
    long m_uncompressedSize;
    long m_currentInflatedCount;
    zng_stream *zStream;
};

#endif // JEMOC_STREAM_TEST_INFLATER_H
