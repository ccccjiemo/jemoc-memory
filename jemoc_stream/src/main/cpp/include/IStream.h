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

#define GET_JS_INFO_WITHOUT_CHECK(count)                                                                               \
    GET_JS_INFO_WITHOUT_STREAM(count)                                                                                  \
    IStream *stream = getStream(env, _this);


#define GET_JS_INFO_WITHOUT_STREAM(count)                                                                              \
    napi_value _this = nullptr;                                                                                        \
    size_t argc = count;                                                                                               \
    napi_value argv[count];                                                                                            \
    void *className;                                                                                                   \
    napi_get_cb_info(env, info, &argc, argv, &_this, &className);                                                      \
    const char *tagName = static_cast<char *>(className);

#define GET_JS_INFO(count)                                                                                             \
    GET_JS_INFO_WITHOUT_STREAM(count)                                                                                  \
    IStream *stream = getStream(env, _this);                                                                           \
    if (stream == nullptr) {                                                                                           \
        napi_throw_error(env, tagName, "stream is null");                                                              \
    }                                                                                                                  \
    if (stream->isClose()) {                                                                                           \
        napi_throw_error(env, tagName, "stream is closed");                                                            \
    }

#define RETURN_NAPI_VALUE(func, value)                                                                                 \
    napi_value result = nullptr;                                                                                       \
    func(env, value, &result);                                                                                         \
    return result;

#define RETURN_BOOL(value) RETURN_NAPI_VALUE(napi_get_boolean, value);

#define DEFINE_ISTREAM_GET_BOOL_FUNCTION(func, func1)                                                                  \
    napi_value IStream::func(napi_env env, napi_callback_info info) {                                                  \
        GET_JS_INFO(0)                                                                                                 \
        RETURN_NAPI_VALUE(napi_get_boolean, stream->func1());                                                          \
    }

#define DEFINE_ISTREAM_GET_STATE(jsfunc, funname)                                                                      \
    napi_value IStream::jsfunc(napi_env env, napi_callback_info info) {                                                \
        GET_JS_INFO_WITHOUT_CHECK(0)                                                                                   \
        bool value = false;                                                                                            \
        if (!(stream == nullptr || stream->isClose())) {                                                               \
            value = stream->funname();                                                                                 \
        }                                                                                                              \
        napi_value result = nullptr;                                                                                   \
        NAPI_CALL(env, napi_get_boolean(env, value, &result))                                                          \
        return result;                                                                                                 \
    }


#define DEFINE_ISTREAM_GET_LONG_FUNCTION(func, func1)                                                                  \
    napi_value IStream::func(napi_env env, napi_callback_info info) {                                                  \
        GET_JS_INFO_WITHOUT_CHECK(0)                                                                                   \
        if (stream == nullptr || stream->isClose()) {                                                                  \
            napi_throw_error(env, tagName, "stream is closed");                                                        \
        }                                                                                                              \
        try {                                                                                                          \
            long resultVal = stream->func1();                                                                          \
            napi_value result = nullptr;                                                                               \
            napi_create_int64(env, resultVal, &result);                                                                \
            return result;                                                                                             \
        } catch (const std::ios_base::failure &e) {                                                                    \
            napi_throw_error(env, tagName, e.what());                                                                  \
        }                                                                                                              \
        return nullptr;                                                                                                \
    }

#define DEFINE_ISTREAM_SET_FUNC(func, func1)                                                                           \
    napi_value IStream::func(napi_env env, napi_callback_info info) {                                                  \
        GET_JS_INFO(1)                                                                                                 \
        long val = 0;                                                                                                  \
        napi_get_value_int64(env, argv[0], &val);                                                                      \
        stream->func1(val);                                                                                            \
        return nullptr;                                                                                                \
    }

#define DEFINE_NAPI_ISTREAM_PROPERTY(className)                                                                        \
    DEFINE_NAPI_FUNCTION("canRead", nullptr, IStream::JSGetCanRead, nullptr, className),                               \
        DEFINE_NAPI_FUNCTION("canWrite", nullptr, IStream::JSGetCanWrite, nullptr, className),                         \
        DEFINE_NAPI_FUNCTION("canSeek", nullptr, IStream::JSGetCanSeek, nullptr, className),                           \
        DEFINE_NAPI_FUNCTION("position", nullptr, IStream::JSGetPosition, nullptr, className),                         \
        DEFINE_NAPI_FUNCTION("length", nullptr, IStream::JSGetLength, IStream::JSSetLength, className),                \
        DEFINE_NAPI_FUNCTION("copyTo", IStream::JSCopyTo, nullptr, nullptr, className),                                \
        DEFINE_NAPI_FUNCTION("seek", IStream::JSSeek, nullptr, nullptr, className),                                    \
        DEFINE_NAPI_FUNCTION("read", IStream::JSRead, nullptr, nullptr, className),                                    \
        DEFINE_NAPI_FUNCTION("write", IStream::JSWrite, nullptr, nullptr, className),                                  \
        DEFINE_NAPI_FUNCTION("flush", IStream::JSFlush, nullptr, nullptr, className),                                  \
        DEFINE_NAPI_FUNCTION("close", IStream::JSClose, nullptr, nullptr, className),                                  \
        DEFINE_NAPI_FUNCTION("readAsync", IStream::JSReadAsync, nullptr, nullptr, className),                          \
        DEFINE_NAPI_FUNCTION("writeAsync", IStream::JSWriteAsync, nullptr, nullptr, className),                        \
        DEFINE_NAPI_FUNCTION("copyToAsync", IStream::JSCopyToAsync, nullptr, nullptr, className),                      \
        DEFINE_NAPI_FUNCTION("flushAsync", IStream::JSFlushAsync, nullptr, nullptr, className),                        \
        DEFINE_NAPI_FUNCTION("closeAsync", IStream::JSCloseAsync, nullptr, nullptr, className),                        \
        DEFINE_NAPI_FUNCTION("isClosed", nullptr, IStream::JSGetIsClosed, nullptr, className)


#define CHECK_STREAM                                                                                                   \
    if (stream->isClose()) {                                                                                           \
        napi_throw_error(env, tagName, "stream is closed");                                                            \
    }

enum SeekOrigin { Begin, Current, End };

class IStream {
public:
    static napi_value JSGetCanRead(napi_env env, napi_callback_info info);
    static napi_value JSGetCanWrite(napi_env env, napi_callback_info info);
    static napi_value JSGetCanSeek(napi_env env, napi_callback_info info);
    static napi_value JSGetPosition(napi_env env, napi_callback_info info);
    static napi_value JSGetLength(napi_env env, napi_callback_info info);
    static napi_value JSSetLength(napi_env env, napi_callback_info info);
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
    static napi_value JSGetIsClosed(napi_env env, napi_callback_info info);
    static napi_value JSCreateInterface(napi_env env, IStream *stream);


public:
    IStream()
        : m_canRead(false), m_canWrite(false), m_canSeek(false), m_position(0), m_length(0), m_canGetLength(false),
          m_canGetPosition(false), m_closed(false) {}
    virtual bool getCanRead() const { return m_canRead; }
    virtual bool getCanWrite() const { return m_canWrite; }
    virtual bool getCanSeek() const { return m_canSeek; }
    virtual long getPosition() const { return m_position; }
    virtual long getLength() const { return m_length; }
    virtual void setLength(long length) { throw std::ios::failure("set length not supported"); }
    virtual void copyTo(IStream *stream, long bufferSize);
    virtual long seek(long offset, SeekOrigin origin);
    virtual void flush() {}
    virtual void close();
    virtual long read(void *buffer, long offset, size_t count) { return 0; };
    virtual long write(void *buffer, long offset, size_t count) { return 0; };
    virtual bool isClose() const { return m_closed; }

protected:
    bool m_canRead;
    bool m_canWrite;
    bool m_canSeek;
    bool m_canGetLength;
    bool m_canGetPosition;
    long m_position;
    long m_length;
    bool m_closed;
};

#endif // JEMOC_STREAM_TEST_ISTREAM_H
