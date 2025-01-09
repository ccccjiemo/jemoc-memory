//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "IStream.h"
#include "common.h"

void IStream::copyTo(IStream *stream, long bufferSize) {
    byte *buffer = new byte[bufferSize];
    long readBytes = 0;
    while ((readBytes = read(buffer, 0, bufferSize)) != 0) {
        stream->write(buffer, 0, readBytes);
    }
    delete[] buffer;
}

long IStream::seek(long offset, SeekOrigin origin) {
    long pos = 0;
    switch (origin) {
    case Begin:
        pos = offset;
        break;
    case Current:
        pos = m_position + offset;
        break;
    case End:
        pos = m_length + offset;
        break;
    default:
        throw std::ios_base::failure("origin is out of range");
    }
    if (pos < 0 || pos > m_length)
        throw std::ios_base::failure("seek error");
    m_position = pos;
    return m_position;
}

void IStream::close() {
    if (m_closed)
        return;
    m_closed = true;
    m_canRead = false;
    m_canWrite = false;
    m_canSeek = false;
    m_canGetLength = false;
}


DEFINE_ISTREAM_GET_BOOL_FUNCTION(JSGetCanRead, getCanRead)
DEFINE_ISTREAM_GET_BOOL_FUNCTION(JSGetCanWrite, getCanWrite)
DEFINE_ISTREAM_GET_BOOL_FUNCTION(JSGetCanSeek, getCanSeek)
DEFINE_ISTREAM_GET_LONG_FUNCTION(JSGetPosition, getPosition)
DEFINE_ISTREAM_GET_LONG_FUNCTION(JSGetLength, getLength)


napi_value IStream::JSCopyTo(napi_env env, napi_callback_info info) {
    GET_JS_INFO(2)
    if (!stream->getCanRead()) {
        napi_throw_error(env, "IStream::copyTo", "stream not readable");
    }
    IStream *target = getStream(env, argv[0]);
    if (!target->getCanWrite()) {
        napi_throw_error(env, "IStream::copyTo", "stream not writeable");
    }
    napi_valuetype type;
    napi_typeof(env, argv[1], &type);
    long bufferSize = 0;
    if (napi_undefined == type) {
        bufferSize = 8192;
    } else {
        bufferSize = getLong(env, argv[1]);
    }
    if (bufferSize < 0) {
        napi_throw_error(env, "IStream::copyTo", "bufferSize is must larget than zero");
    }
    try {
        stream->copyTo(target, bufferSize);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "IStream::copyTo", e.what());
    }
    return nullptr;
}

napi_value IStream::JSSeek(napi_env env, napi_callback_info info) {
    GET_JS_INFO(2)
    if (!stream->getCanSeek()) {
        napi_throw_error(env, "IStream::seek", "stream not seekable");
    }
    long pos = getLong(env, argv[0]);
    int origin = getInt(env, argv[1]);
    long seekResult = 0;
    try {
        seekResult = stream->seek(pos, SeekOrigin(origin));
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "IStream::seek", e.what());
    }
    RETURN_NAPI_VALUE(napi_create_int64, seekResult)
}

napi_value IStream::JSRead(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanRead()) {
        napi_throw_error(env, "IStream::read", "stream not readable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, "IStream::read", "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);
    long readBytes = 0;
    try {
        readBytes = stream->read(data, offset, count);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "IStream::read", e.what());
    }
    RETURN_NAPI_VALUE(napi_create_int64, readBytes)
}

napi_value IStream::JSWrite(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanWrite()) {
        napi_throw_error(env, "IStream::wirte", "stream not writeable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, "IStream::write", "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);
    long readBytes = 0;
    try {
        readBytes = stream->write(data, offset, count);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "IStream::write", e.what());
    }
    RETURN_NAPI_VALUE(napi_create_int64, readBytes)
}

napi_value IStream::JSFlush(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0);
    try {
        stream->flush();

    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "IStream::flush", e.what());
    }
    return nullptr;
}

napi_value IStream::JSClose(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0);
    stream->close();
    return nullptr;
}

napi_value IStream::JSReadAsync(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanRead()) {
        napi_throw_error(env, "IStream::read", "stream not readable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, "IStream::read", "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);


    napi_value promise = nullptr;
    napi_value resouce_name = nullptr;
    napi_create_string_utf8(env, "readAsync", NAPI_AUTO_LENGTH, &resouce_name);
    AsyncWorkData *asyncData = new AsyncWorkData{.buffer = data, .offset = offset, .count = count, .stream = stream};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->result = asyncData->stream->read(asyncData->buffer, asyncData->offset, asyncData->count);
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            napi_value result = nullptr;
            if (status == napi_ok) {
                napi_create_int64(env, asyncData->result, &result);
                napi_resolve_deferred(env, asyncData->deferred, result);
            } else {
                napi_create_string_utf8(env, "io error", NAPI_AUTO_LENGTH, &result);
                napi_reject_deferred(env, asyncData->deferred, result);
            }
            napi_delete_async_work(env, asyncData->work);
            delete asyncData;
        },
        asyncData, &asyncData->work);

    napi_queue_async_work(env, asyncData->work);
    return promise;
}

napi_value IStream::JSWriteAsync(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanWrite()) {
        napi_throw_error(env, "IStream::wirte", "stream not writeable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, "IStream::write", "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);

    napi_value promise = nullptr;
    napi_value resouce_name = nullptr;
    napi_create_string_utf8(env, "writeAsync", NAPI_AUTO_LENGTH, &resouce_name);
    AsyncWorkData *asyncData = new AsyncWorkData{.buffer = data, .offset = offset, .count = count, .stream = stream};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->result = asyncData->stream->write(asyncData->buffer, asyncData->offset, asyncData->count);
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            napi_value result = nullptr;
            if (status == napi_ok) {
                napi_create_int64(env, asyncData->result, &result);
                napi_resolve_deferred(env, asyncData->deferred, result);
            } else {
                napi_create_string_utf8(env, "io error", NAPI_AUTO_LENGTH, &result);
                napi_reject_deferred(env, asyncData->deferred, result);
            }
            napi_delete_async_work(env, asyncData->work);
            delete asyncData;
        },
        asyncData, &asyncData->work);

    napi_queue_async_work(env, asyncData->work);
    return promise;
}

napi_value IStream::JSCopyToAsync(napi_env env, napi_callback_info info) {
    GET_JS_INFO(2)
    if (!stream->getCanRead()) {
        napi_throw_error(env, "IStream::copyTo", "stream not readable");
    }
    IStream *target = getStream(env, argv[0]);
    if (!target->getCanWrite()) {
        napi_throw_error(env, "IStream::copyTo", "stream not writeable");
    }
    napi_valuetype type;
    napi_typeof(env, argv[1], &type);
    long bufferSize = 0;
    if (napi_undefined == type) {
        bufferSize = 8192;
    } else {
        bufferSize = getLong(env, argv[1]);
    }
    if (bufferSize < 0) {
        napi_throw_error(env, "IStream::copyTo", "bufferSize is must larget than zero");
    }

    napi_value promise = nullptr;
    napi_value resouce_name = nullptr;
    napi_create_string_utf8(env, "copyToAsync", NAPI_AUTO_LENGTH, &resouce_name);
    AsyncWorkData *asyncData = new AsyncWorkData{.bufferSize = bufferSize, .stream = stream, .targetStream = target};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->stream->copyTo(asyncData->targetStream, asyncData->bufferSize);
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            napi_value result = nullptr;
            if (status == napi_ok) {
                napi_get_undefined(env, &result);
                napi_resolve_deferred(env, asyncData->deferred, result);
            } else {
                napi_create_string_utf8(env, "io error", NAPI_AUTO_LENGTH, &result);
                napi_reject_deferred(env, asyncData->deferred, result);
            }
            napi_delete_async_work(env, asyncData->work);
            delete asyncData;
        },
        asyncData, &asyncData->work);

    napi_queue_async_work(env, asyncData->work);
    return promise;
}


napi_value IStream::JSFlushAsync(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0);
    napi_value promise = nullptr;
    napi_value resouce_name = nullptr;
    napi_create_string_utf8(env, "flushAsync", NAPI_AUTO_LENGTH, &resouce_name);
    AsyncWorkData *asyncData = new AsyncWorkData{.stream = stream};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->stream->flush();
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            napi_value result = nullptr;
            if (status == napi_ok) {
                napi_get_undefined(env, &result);
                napi_resolve_deferred(env, asyncData->deferred, result);
            } else {
                napi_create_string_utf8(env, "io error", NAPI_AUTO_LENGTH, &result);
                napi_reject_deferred(env, asyncData->deferred, result);
            }
            napi_delete_async_work(env, asyncData->work);
            delete asyncData;
        },
        asyncData, &asyncData->work);

    napi_queue_async_work(env, asyncData->work);
    return promise;
}

napi_value IStream::JSCloseAsync(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0);
    napi_value promise = nullptr;
    napi_value resouce_name = nullptr;
    napi_create_string_utf8(env, "closeAsync", NAPI_AUTO_LENGTH, &resouce_name);
    AsyncWorkData *asyncData = new AsyncWorkData{.stream = stream};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->stream->close();
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            napi_value result = nullptr;
            if (status == napi_ok) {
                napi_get_undefined(env, &result);
                napi_resolve_deferred(env, asyncData->deferred, result);
            } else {
                napi_create_string_utf8(env, "io error", NAPI_AUTO_LENGTH, &result);
                napi_reject_deferred(env, asyncData->deferred, result);
            }
            napi_delete_async_work(env, asyncData->work);
            delete asyncData;
        },
        asyncData, &asyncData->work);

    napi_queue_async_work(env, asyncData->work);
    return promise;
}
