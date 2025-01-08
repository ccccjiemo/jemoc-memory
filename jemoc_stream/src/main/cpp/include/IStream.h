//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ISTREAM_H
#define JEMOC_STREAM_TEST_ISTREAM_H


#include "common.h"
#include <cstddef>
#include <ios>
#include <napi/native_api.h>

typedef unsigned char byte;

class IStream;

struct AsyncWorkData {
    void *buffer;
    long offset;
    long count;
    long result;
    long bufferSize;
    IStream *stream;
    IStream *targetStream;
    napi_deferred deferred;
    napi_async_work work;
};


#define GET_JS_INFO_WITHOUT_STREAM(count)                                                                              \
    napi_value _this = nullptr;                                                                                        \
    size_t argc = count;                                                                                               \
    napi_value argv[count];                                                                                            \
    napi_get_cb_info(env, info, &argc, argv, &_this, nullptr);

#define GET_JS_INFO(count)                                                                                             \
    GET_JS_INFO_WITHOUT_STREAM(count)                                                                                  \
    IStream *stream = getStream(env, _this);                                                                           \
    if (stream == nullptr) {                                                                                           \
        napi_throw_error(env, "IStream", "stream is null");                                                            \
    }

#define RETURN_NAPI_VALUE(func, value)                                                                                 \
    napi_value result = nullptr;                                                                                       \
    func(env, value, &result);                                                                                         \
    return result;

#define RETURN_BOOL(value) RETURN_NAPI_VALUE(napi_get_boolean, value);

#define DEFINE_ISTREAM_GET_FUNC(func, func1)                                                                           \
    napi_value IStream::func(napi_env env, napi_callback_info info) {                                                  \
        GET_JS_INFO(0)                                                                                                 \
        RETURN_BOOL(stream->func1());                                                                                  \
    }

#define DEFINE_ISTREAM_SET_FUNC(func, func1)                                                                           \
    napi_value IStream::func(napi_env env, napi_callback_info info) {                                                  \
        GET_JS_INFO(1)                                                                                                 \
        long val = 0;                                                                                                  \
        napi_get_value_int64(env, argv[0], &val);                                                                      \
        stream->func1(val);                                                                                            \
        return nullptr;                                                                                                \
    }

#define DEFINE_NAPI_ISTREAM_PROPERTY                                                                                   \
    DEFINE_NAPI_FUNCTION("canRead", nullptr, IStream::JSGetCanRead, nullptr),                                          \
        DEFINE_NAPI_FUNCTION("canWrite", nullptr, IStream::JSGetCanWrite, nullptr),                                    \
        DEFINE_NAPI_FUNCTION("canSeek", nullptr, IStream::JSGetCanSeek, nullptr),                                      \
        DEFINE_NAPI_FUNCTION("position", nullptr, IStream::JSGetPosition, IStream::JSSetPosition),                     \
        DEFINE_NAPI_FUNCTION("length", nullptr, IStream::JSGetLength, nullptr),                                        \
        DEFINE_NAPI_FUNCTION("copyTo", IStream::JSCopyTo, nullptr, nullptr),                                           \
        DEFINE_NAPI_FUNCTION("seek", IStream::JSSeek, nullptr, nullptr),                                               \
        DEFINE_NAPI_FUNCTION("read", IStream::JSRead, nullptr, nullptr),                                               \
        DEFINE_NAPI_FUNCTION("write", IStream::JSWrite, nullptr, nullptr),                                             \
        DEFINE_NAPI_FUNCTION("flush", IStream::JSFlush, nullptr, nullptr),                                             \
        DEFINE_NAPI_FUNCTION("close", IStream::JSClose, nullptr, nullptr),                                             \
        DEFINE_NAPI_FUNCTION("readAsync", IStream::JSReadAsync, nullptr, nullptr),                                     \
        DEFINE_NAPI_FUNCTION("writeAsync", IStream::JSWriteAsync, nullptr, nullptr),                                   \
        DEFINE_NAPI_FUNCTION("copyToAsync", IStream::JSCopyToAsync, nullptr, nullptr),                                 \
        DEFINE_NAPI_FUNCTION("flushAsync", IStream::JSFlushAsync, nullptr, nullptr),                                   \
        DEFINE_NAPI_FUNCTION("closeAsync", IStream::JSCloseAsync, nullptr, nullptr)


enum SeekOrigin { Begin, Current, End };

class IStream {
public:
    static napi_value JSGetCanRead(napi_env env, napi_callback_info info);
    static napi_value JSGetCanWrite(napi_env env, napi_callback_info info);
    static napi_value JSGetCanSeek(napi_env env, napi_callback_info info);
    static napi_value JSGetPosition(napi_env env, napi_callback_info info);
    static napi_value JSSetPosition(napi_env env, napi_callback_info info);
    static napi_value JSGetLength(napi_env env, napi_callback_info info);
    static napi_value JSSeek(napi_env env, napi_callback_info info);
    static napi_value JSRead(napi_env env, napi_callback_info info);
    static napi_value JSWrite(napi_env env, napi_callback_info info);
    static napi_value JSFlush(napi_env env, napi_callback_info info);
    static napi_value JSCopyTo(napi_env env, napi_callback_info info);
    static napi_value JSClose(napi_env env, napi_callback_info info);
    static napi_value JSReadAsync(napi_env env, napi_callback_info info);
    static napi_value JSWriteAsync(napi_env env, napi_callback_info info);
    static napi_value JSCopyToAsync(napi_env env, napi_callback_info info);
    static napi_value JSFlushAsync(napi_env env, napi_callback_info info);
    static napi_value JSCloseAsync(napi_env env, napi_callback_info info);


public:
    IStream()
        : m_canRead(true), m_canWrite(true), m_canSeek(true), m_position(0), m_length(0), m_canGetLength(true),
          m_closed(false) {}
    virtual bool getCanRead() const { return m_canRead; }
    virtual bool getCanWrite() const { return m_canWrite; }
    virtual bool getCanSeek() const { return m_canSeek; }
    virtual long getPosition() const { return m_position; }
    virtual void setPosition(const long pos) { m_position = pos; }
    virtual long getLength() const { return m_length; }
    virtual void copyTo(IStream *stream, long bufferSize);
    virtual long seek(long offset, SeekOrigin origin);
    virtual void flush() {}
    virtual void close();
    virtual long read(void *buffer, long offset, size_t count){};
    virtual long write(void *buffer, long offset, size_t count){};

protected:
    bool m_canRead;
    bool m_canWrite;
    bool m_canSeek;
    bool m_canGetLength;
    long m_position;
    long m_length;
    bool m_closed;
};

#endif // JEMOC_STREAM_TEST_ISTREAM_H
