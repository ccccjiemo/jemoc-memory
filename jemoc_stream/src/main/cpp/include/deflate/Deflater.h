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
#include <napi/native_api.h>

#define Min_WINDOW_BITS -15
#define Max_WINDOW_BITS 31

#define GET_DEFLATER_INFO(number)                                                                                      \
    size_t argc = number;                                                                                              \
    napi_value argv[number];                                                                                           \
    napi_value _this = nullptr;                                                                                        \
    napi_get_cb_info(env, info, &argc, argv, &_this, nullptr);

#define GET_DEFLATER_INFO_WITH_DEFLATER(number)                                                                        \
    GET_DEFLATER_INFO(number)                                                                                          \
    void *p = nullptr;                                                                                                 \
    napi_unwrap(env, _this, &p);                                                                                       \
    if (p == nullptr)                                                                                                  \
        napi_throw_error(env, "Deflater", "deflater is disposed");                                                     \
    Deflater *deflater = static_cast<Deflater *>(p);

class Deflater {
public:
    Deflater(int windowBits, int level, int strategy);
    ~Deflater();

    void setInput(void *buffer, size_t count);
    bool needInput() const;
    bool finish(void *buffer, size_t count, size_t *bytesRead);
    long getDeflateOutput(void *buffer, size_t count);
    bool flush(void *buffer, size_t count, size_t *bytesRead);

public:
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static napi_value JSSetInput(napi_env env, napi_callback_info info);
    static napi_value JSNeedInput(napi_env env, napi_callback_info info);
    static napi_value JSDispose(napi_env env, napi_callback_info info);
    static napi_value JSIsDisposed(napi_env env, napi_callback_info info);
    static napi_value JSFlush(napi_env env, napi_callback_info info);
    static napi_value JSFinish(napi_env evn, napi_callback_info info);
    static napi_value JSDeflate(napi_env env, napi_callback_info info);
    static void Export(napi_env env, napi_value exports);

private:
    int readDeflateOutput(void *buffer, size_t count, int flushCode, size_t *bytesRead);
    int deflate(int flushCode);

private:
    int m_windowBits;
    int m_level;
    int m_strategy;
    zng_stream *zStream;
    std::mutex mtx;
};

#endif // JEMOC_STREAM_TEST_DEFLATER_H
