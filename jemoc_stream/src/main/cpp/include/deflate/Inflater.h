//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_INFLATER_H
#define JEMOC_STREAM_TEST_INFLATER_H
#include <napi/native_api.h>
#include "zlib-ng.h"

#define GZIP_Header_ID1 31
#define GZIP_Header_ID2 139

#define GET_INFLATER_INFO(number)                                                                                      \
    size_t argc = number;                                                                                              \
    napi_value argv[number];                                                                                           \
    napi_value _this = nullptr;                                                                                        \
    napi_get_cb_info(env, info, &argc, argv, &_this, nullptr);

#define GET_INFLATER_INFO_WITH_INFLATER(number)                                                                        \
    GET_INFLATER_INFO(number)                                                                                          \
    void *p = nullptr;                                                                                                 \
    napi_unwrap(env, _this, &p);                                                                                       \
    if (p == nullptr)                                                                                           \
        napi_throw_error(env, "Inflater", "inflater is disposed");                                                     \
    Inflater *inflater = static_cast<Inflater *>(p);

class Inflater {
public:
    Inflater(int windowBits, long uncompressedSize = -1);
    ~Inflater();

    bool isFinished() const;
    long inflate(void *buffer, size_t count);
    bool needInput() const;
    bool isGzipStream() const;
    void setInput(void *buffer, size_t count);

public:
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static napi_value JSGetIsFinished(napi_env env, napi_callback_info info);
    static napi_value JSInflate(napi_env env, napi_callback_info info);
    static napi_value JSGetIsGzipStream(napi_env env, napi_callback_info info);
    static napi_value JSSetInput(napi_env env, napi_callback_info info);
    static napi_value JSNeedInput(napi_env env, napi_callback_info info);
    static napi_value JSDispose(napi_env env, napi_callback_info info);
    static napi_value JSIsDisposed(napi_env env, napi_callback_info info);
    static void Export(napi_env env, napi_value exports);

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
