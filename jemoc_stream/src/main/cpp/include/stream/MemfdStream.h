//
// Created on 2025/2/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_MEMFDSTREAM_H
#define JEMOC_STREAM_TEST_MEMFDSTREAM_H

#include "IStream.h"
#include <stdexcept>

// 基于 memfd_create 的内存流实现，支持截断流长度，并允许构造时传入初始缓冲区数据
class MemfdStream : public IStream {
public:
    MemfdStream();
    MemfdStream(const void *initialBuffer, size_t bufferSize);
    virtual ~MemfdStream();

    long read(void *buffer, long offset, size_t count) override;
    long write(void *buffer, long offset, size_t count) override;
    long seek(long offset, SeekOrigin origin) override;
    void flush() override;
    void close() override;
    void setLength(long length) override;

    // 获取 memfd 的文件描述符
    int getFd() const;

public:
    static std::string ClassName;
    static napi_ref cons;
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDisposed(napi_env env, void *data, void *hint);
    static napi_value JSToArrayBuffer(napi_env env, napi_callback_info info);
    static void Export(napi_env env, napi_value exports);
    static napi_value JSGetFd(napi_env env, napi_callback_info info);
    napi_value readAllFromFd(napi_env env);

private:
    int m_fd;
};


#endif // JEMOC_STREAM_TEST_MEMFDSTREAM_H
