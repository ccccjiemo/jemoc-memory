//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_MEMORYSTREAM_H
#define JEMOC_STREAM_TEST_MEMORYSTREAM_H

#include "IStream.h"
#include <vector>


class MemoryStream : public IStream {
public:
    MemoryStream();
    ~MemoryStream();
    long read(void *buffer, long offset, size_t count) override;
    long write(void *buffer, long offset, size_t count) override;
    void setCapacity(long capacity);
    long getCapacity() const;
    void close() override;


public:
    static std::string ClassName;
    static napi_ref cons;
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDisposed(napi_env env, void *data, void *hint);
    static napi_value JSToArrayBuffer(napi_env env, napi_callback_info info);
    static void Export(napi_env env, napi_value exports);
    static napi_value JSGetCapacity(napi_env env, napi_callback_info info);
    static napi_value JSSetCapacity(napi_env env, napi_callback_info info);

private:
    void ensureCapacity(long capacity);

private:
    std::vector<byte> m_cache;
    long m_capacity;
};


#endif // JEMOC_STREAM_TEST_MEMORYSTREAM_H
