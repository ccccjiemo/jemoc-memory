//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Deflater.h"
#include <ios>

static int getMemLevel(int level) { return level == Z_NO_COMPRESSION ? 7 : 8; }

Deflater::Deflater(int windowBits, int level, int strategy)
    : m_windowBits(windowBits), m_level(level), m_strategy(strategy) {
    if (windowBits < Min_WINDOW_BITS || windowBits > Max_WINDOW_BITS)
        throw std::ios_base::failure("deflater: windowbits must be greater than -15 and less than 31. ");
    zStream = new zng_stream{.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL};
    int errCode = zng_deflateInit2(zStream, m_level, Z_DEFLATED, m_windowBits, getMemLevel(m_level), m_strategy);


    if (errCode == Z_OK)
        return;

    std::string err;
    err = "deflater: deflateInit2 failed";
    err += zStream->msg;

    delete zStream;
    zStream = nullptr;

    throw std::ios_base::failure(err);
}

Deflater::~Deflater() {
    if (zStream != nullptr) {
        zng_deflateEnd(zStream);
        delete zStream;
        zStream = nullptr;
    }
}

bool Deflater::needInput() const { return zStream->avail_in == 0; }
void Deflater::setInput(void *buffer, size_t count) {
    mtx.lock();
    zStream->avail_in = count;
    zStream->next_in = static_cast<const uint8_t *>(buffer);
    mtx.unlock();
}

long Deflater::getDeflateOutput(void *buffer, size_t count) {
    size_t bytesRead = 0;
    readDeflateOutput(buffer, count, Z_NO_FLUSH, &bytesRead);
    return bytesRead;
}

int Deflater::readDeflateOutput(void *buffer, size_t count, int flushCode, size_t *bytesRead) {
    mtx.lock();
    zStream->next_out = static_cast<uint8_t *>(buffer);
    zStream->avail_out = count;
    int errCode = deflate(flushCode);
    *bytesRead = count - zStream->avail_out;
    mtx.unlock();
    return errCode;
}

int Deflater::deflate(int flushCode) {
    int errCode = zng_deflate(zStream, flushCode);

    switch (errCode) {
    case Z_OK:
    case Z_STREAM_END:
        return errCode;
    case Z_BUF_ERROR:
        return errCode;
    case Z_STREAM_ERROR:
        throw std::ios_base::failure("deflater: deflate failed, inconsistent stream ");
    default:
        throw std::ios_base::failure(std::string("deflater: deflate failed, ") + zStream->msg);
    }
}

bool Deflater::finish(void *buffer, size_t count, size_t *bytesRead) {
    int errCode = readDeflateOutput(buffer, count, Z_FINISH, bytesRead);
    return errCode == Z_STREAM_END;
}

bool Deflater::flush(void *buffer, size_t count, size_t *bytesRead) {
    return readDeflateOutput(buffer, count, Z_SYNC_FLUSH, bytesRead) == Z_OK;
}
