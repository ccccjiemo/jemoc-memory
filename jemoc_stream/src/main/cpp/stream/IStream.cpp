//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "IStream.h"
#include "common.h"

napi_value IStream::cons = nullptr;
std::string IStream::ClassName = "StreamBase";

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
    m_canSetLength = false;
}


DEFINE_ISTREAM_GET_STATE(JSGetCanRead, getCanRead)
DEFINE_ISTREAM_GET_STATE(JSGetCanWrite, getCanWrite)
DEFINE_ISTREAM_GET_STATE(JSGetCanSeek, getCanSeek)
DEFINE_ISTREAM_GET_LONG_FUNCTION(JSGetPosition, getPosition)
DEFINE_ISTREAM_GET_LONG_FUNCTION(JSGetLength, getLength)


napi_value IStream::JSCopyTo(napi_env env, napi_callback_info info) {
    GET_JS_INFO(2)
    if (!stream->getCanRead()) {
        napi_throw_error(env, ClassName.c_str(), "stream not readable");
    }
    std::shared_ptr<IStream> target = GetStream(env, argv[0]);
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[1], &type))
    long bufferSize = 0;
    if (napi_undefined == type) {
        bufferSize = 8192;
    } else {
        bufferSize = getLong(env, argv[1]);
    }
    if (bufferSize < 0) {
        NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "bufferSize is must larget than zero"))
        return nullptr;
    }

    if (!stream->getCanRead()) {
        NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "source stream not writeable"))
        return nullptr;
    }

    if (target) {
        if (target->isClose()) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "target stream is closed"));
            return nullptr;
        }
        if (!target->getCanWrite()) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "target stream not writeable"))
            return nullptr;
        }

        try {
            stream->copyTo(target.get(), bufferSize);
        } catch (const std::ios_base::failure &e) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), e.what()))
        }
    } else {
        napi_handle_scope scope;
        napi_value jsStatus = nullptr;
        bool status = false;
        NAPI_CALL(env, napi_get_named_property(env, argv[0], "isClosed", &jsStatus))
        NAPI_CALL(env, napi_get_value_bool(env, jsStatus, &status))
        if (!status) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "target stream is closed"))
            return nullptr;
        }
        NAPI_CALL(env, napi_get_named_property(env, argv[0], "canWrite", &jsStatus))
        NAPI_CALL(env, napi_get_value_bool(env, jsStatus, &status))
        if (!status) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "target stream not writeable"))
            return nullptr;
        }
        napi_value writeFunc = nullptr;
        NAPI_CALL(env, napi_get_named_property(env, argv[0], "write", &writeFunc))
        NAPI_CALL(env, napi_typeof(env, writeFunc, &type))

        if (type != napi_function) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), "target function is not callable"))
        }

        NAPI_CALL(env, napi_open_handle_scope(env, &scope)) napi_value jsbuffer = nullptr;
        void *buffer = nullptr;
        NAPI_CALL(env, napi_create_arraybuffer(env, bufferSize, &buffer, &jsbuffer))
        long readBytes = 0;
        napi_value jsReadBytes = nullptr;
        napi_value jsOffset = nullptr;
        NAPI_CALL(env, napi_create_int32(env, 0, &jsOffset))
        std::string catch_error;
        while ((readBytes = stream->read(buffer, 0, bufferSize)) > 0) {
            NAPI_CALL(env, napi_create_int64(env, readBytes, &jsReadBytes))
            napi_value targetArgv[3] = {jsbuffer, jsOffset, jsReadBytes};
            if (napi_call_function(env, argv[0], writeFunc, 3, targetArgv, nullptr) == napi_pending_exception) {
                napi_value error;
                napi_get_and_clear_last_exception(env, &error);
                napi_value errorMessage;
                napi_coerce_to_string(env, error, &errorMessage);
                char message[1024];
                size_t len;
                napi_get_value_string_utf8(env, errorMessage, message, sizeof(message), &len);
                catch_error = message;
                break;
            }
        }
        NAPI_CALL(env, napi_close_handle_scope(env, scope))
        if (!catch_error.empty()) {
            NAPI_CALL(env, napi_throw_error(env, ClassName.c_str(), catch_error.c_str()))
        }
    }

    return nullptr;
}

napi_value IStream::JSSeek(napi_env env, napi_callback_info info) {
    GET_JS_INFO(2)
    try {
        if (!stream->getCanSeek()) {
            napi_throw_error(env, ClassName.c_str(), "stream not seekable");
        }
        long pos = getLong(env, argv[0]);
        int origin = getInt(env, argv[1]);
        long seekResult = 0;
        seekResult = stream->seek(pos, SeekOrigin(origin));
        RETURN_NAPI_VALUE(napi_create_int64, seekResult)

    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
        return nullptr;
    }
}

napi_value IStream::JSRead(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanRead()) {
        napi_throw_error(env, ClassName.c_str(), "stream not readable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, ClassName.c_str(), "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);
    long readBytes = 0;
    try {
        readBytes = stream->read(data, offset, count);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }
    RETURN_NAPI_VALUE(napi_create_int64, readBytes)
}

napi_value IStream::JSSetLength(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    long length = getLong(env, argv[0]);

    if (!stream->m_canSetLength)
        napi_throw_error(env, ClassName.c_str(), "stream not supported set length");

    try {
        stream->setLength(length);
    } catch (const std::exception &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }
    return nullptr;
}


napi_value IStream::JSWrite(napi_env env, napi_callback_info info) {
    GET_JS_INFO(3)
    if (!stream->getCanWrite()) {
        napi_throw_error(env, ClassName.c_str(), "stream not writeable");
    }
    void *data = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &data, &length);
    if (data == nullptr) {
        napi_throw_type_error(env, ClassName.c_str(), "buffer is null");
        return nullptr;
    }
    long offset = getOffset(env, argv[1], length);
    long count = getCount(env, argv[2], length, offset);
    long readBytes = 0;
    try {
        readBytes = stream->write(data, offset, count);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }
    RETURN_NAPI_VALUE(napi_create_int64, readBytes)
}

napi_value IStream::JSFlush(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0);
    try {
        stream->flush();

    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }
    return nullptr;
}

napi_value IStream::JSClose(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(0)
    std::shared_ptr<IStream> stream = GetStream(env, _this);
    if (!stream) {
        return nullptr;
    }
    stream->close(env);
    void *result = nullptr;
    napi_remove_wrap(env, _this, &result);

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
    AsyncWorkData *asyncData =
        new AsyncWorkData{.buffer = data, .offset = offset, .count = count, .stream = stream.get()};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            std::lock_guard<std::mutex> lock(asyncData->stream->mutex_);
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
    AsyncWorkData *asyncData =
        new AsyncWorkData{.buffer = data, .offset = offset, .count = count, .stream = stream.get()};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            std::lock_guard<std::mutex> lock(asyncData->stream->mutex_);
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
    std::shared_ptr<IStream> target = GetStream(env, argv[0]);
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
    AsyncWorkData *asyncData =
        new AsyncWorkData{.bufferSize = bufferSize, .stream = stream.get(), .targetStream = target.get()};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            std::lock_guard<std::mutex> lock(asyncData->stream->mutex_);
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
    AsyncWorkData *asyncData = new AsyncWorkData{.stream = stream.get()};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            std::lock_guard<std::mutex> lock(asyncData->stream->mutex_);
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
    AsyncWorkData *asyncData = new AsyncWorkData{.stream = stream.get()};
    napi_create_promise(env, &asyncData->deferred, &promise);
    napi_create_async_work(
        env, nullptr, resouce_name,
        [](napi_env env, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            asyncData->stream->close();
        },
        [](napi_env env, napi_status status, void *data) {
            AsyncWorkData *asyncData = static_cast<AsyncWorkData *>(data);
            std::lock_guard<std::mutex> lock(asyncData->stream->mutex_);
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

napi_value IStream::JSCreateInterface(napi_env env, std::shared_ptr<IStream> stream) {
    napi_value result = nullptr;
    napi_property_descriptor desc[] = {DEFINE_NAPI_ISTREAM_PROPERTY(nullptr)};
    SharedPtrWrapper *wrapper = new SharedPtrWrapper(stream);
    NAPI_CALL(env, napi_create_object_with_properties(env, &result, sizeof(desc) / sizeof(desc[0]), desc))
    NAPI_CALL(env, napi_wrap(
                       env, result, wrapper,
                       [](napi_env env, void *data, void *hint) {
                           SharedPtrWrapper *wrapper = static_cast<SharedPtrWrapper *>(data);
                           delete wrapper;
                       },
                       nullptr, nullptr))
    return result;
}

napi_value IStream::JSBind(napi_env env, napi_value value, std::shared_ptr<IStream> stream) {
//    NAPI_CALL(env,)
    napi_wrap(
        env, value, new SharedPtrWrapper(stream),
        [](napi_env env, void *data, void *hint) {
            if (data) {
                delete static_cast<SharedPtrWrapper *>(data);
            }
        },
        nullptr, nullptr);
    return value;
}


napi_value IStream::JSGetIsClosed(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_CHECK(0)

    bool value = false;
    try {
        value = stream == nullptr || stream->isClose();
    } catch (const std::exception &e) {
        value = true;
    }
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result))
    return result;
}


void IStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_FUNCTION("canRead", nullptr, IStream::JSGetCanRead, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("canWrite", nullptr, IStream::JSGetCanWrite, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("canSeek", nullptr, IStream::JSGetCanSeek, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("position", nullptr, IStream::JSGetPosition, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("length", nullptr, IStream::JSGetLength, IStream::JSSetLength, nullptr),
        DEFINE_NAPI_FUNCTION("copyTo", IStream::JSCopyTo, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("seek", IStream::JSSeek, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("read", IStream::JSRead, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("write", IStream::JSWrite, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("flush", IStream::JSFlush, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("close", IStream::JSClose, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("readAsync", IStream::JSReadAsync, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("writeAsync", IStream::JSWriteAsync, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("copyToAsync", IStream::JSCopyToAsync, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("flushAsync", IStream::JSFlushAsync, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("closeAsync", IStream::JSCloseAsync, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isClosed", nullptr, IStream::JSGetIsClosed, nullptr, nullptr),
    };
    NAPI_CALL(env, napi_define_class(
                       env, ClassName.c_str(), NAPI_AUTO_LENGTH,
                       [](napi_env env, napi_callback_info info) -> napi_value { return nullptr; }, nullptr,
                       sizeof(desc) / sizeof(desc[0]), desc, &cons));

    NAPI_CALL(env, napi_set_named_property(env, exports, ClassName.c_str(), cons))
}

void IStream::Extends(napi_env env, napi_value constructor) {
    napi_value baseCon = nullptr;
    napi_value extendCon = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, cons, "prototype", &baseCon))
    NAPI_CALL(env, napi_get_named_property(env, constructor, "prototype", &extendCon))
    NAPI_CALL(env, napi_set_named_property(env, extendCon, "__proto__", baseCon))
}

IStream::SharedPtrWrapper *IStream::MakePtr(IStream *stream) {
    return new IStream::SharedPtrWrapper(std::shared_ptr<IStream>(stream));
}

std::shared_ptr<IStream> IStream::GetStream(napi_env env, napi_value value) {
    void *result = nullptr;
    NAPI_CALL(env, napi_unwrap(env, value, &result))
    if (result == nullptr)
        return nullptr;
    return static_cast<SharedPtrWrapper *>(result)->ptr;
}
