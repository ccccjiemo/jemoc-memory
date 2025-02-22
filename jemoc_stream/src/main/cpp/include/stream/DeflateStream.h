//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_DEFLATESTREAM_H
#define JEMOC_STREAM_TEST_DEFLATESTREAM_H

#include "IStream.h"
#include "common.h"
#include "deflate/Deflater.h"
#include "deflate/Inflater.h"
#include <napi/native_api.h>

#define DEFAULT_BUFFER_SIZE 8192


enum DeflateMode { DeflateMode_Compress, DeflateMode_Decompress };

class DeflateStream : public IStream {
public:
    DeflateStream(std::shared_ptr<IStream> stream, DeflateMode mode, int windowBits, int compressionLevel,
                  bool leaveOpen, size_t bufferSize = 8192, long uncompressSize = -1);
    ~DeflateStream();

    void close() override;
    void flush() override;
    long read(void *buffer, long offset, size_t count) override;
    long write(void *buffer, long offset, size_t count) override;
    long getPosition() const override;
    long getLength() const override;
    long seek(long offset, SeekOrigin origin) override;
    void close(napi_env env) override;

    static std::string ClassName;
    static napi_ref cons;
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static void Export(napi_env env, napi_value exports);

protected:
    napi_ref stream_weak_ref = nullptr;

private:
    bool inflaterIsFinished() const;
    void writeDeflaterOutput();
    void flushBuffers();
    void purgeBuffers();

private:
    bool m_wroteBytes = false;
    bool m_leaveOpen;
    int m_windowBits;
    long m_uncompressSize;
    int m_compressionLevel;
    size_t m_bufferSize;
    DeflateMode m_mode;
    std::shared_ptr<IStream> m_stream;
    Deflater *deflater = nullptr;
    Inflater *inflater = nullptr;
    void *m_buffer = nullptr;
};

#endif // JEMOC_STREAM_TEST_DEFLATESTREAM_H
