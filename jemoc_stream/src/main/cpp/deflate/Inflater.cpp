//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "deflate/Inflater.h"
#include "common.h"
#include <cstddef>
#include <ios>
#include <string>

Inflater::Inflater(int windowBits, long uncompressedSize)
    : m_windowBits(windowBits), m_uncompressedSize(uncompressedSize) {
    m_finished = false;
    m_currentInflatedCount = 0;
    zStream = new zng_stream{.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL};
    if (Z_OK != zng_inflateInit2(zStream, m_windowBits)) {
        delete zStream;
        throw std::ios_base::failure("Inflater: failed to initialize zstream.");
    }
}

Inflater::~Inflater() {
    if (zStream != nullptr) {
        zng_inflateEnd(zStream);
        delete zStream;
        zStream = nullptr;
    }
}

bool Inflater::isFinished() const { return m_finished; }
bool Inflater::needInput() const { return zStream->avail_in == 0; }
bool Inflater::isGzipStream() const { return m_windowBits >= 24 && m_windowBits <= 31; }

void Inflater::setInput(void *buffer, size_t count) {
    zStream->next_in = static_cast<const uint8_t *>(buffer);
    zStream->avail_in = count;
}


long Inflater::inflate(void *buffer, size_t count) {
    long bytesRead = 0;
    if (m_uncompressedSize == -1) {
        bytesRead = readOutput(buffer, count);
    } else {
        if (m_uncompressedSize > m_currentInflatedCount) {
            size_t len = std::min(count, (size_t)(m_uncompressedSize - m_currentInflatedCount));
            bytesRead = readOutput(buffer, len);
        } else {
            m_finished = true;
            zStream->avail_in = 0;
        }
    }
    return bytesRead;
}

long Inflater::readOutput(void *buffer, size_t count) {
    int state = 0;
    size_t readout = readInflateOutput(buffer, count, Z_NO_FLUSH, &state);
    if (state == Z_STREAM_END) {
        if (needInput() && isGzipStream()) {
            m_finished = resetStreamForLeftoverInput();
        } else {
            m_finished = true;
        }
    }
    return readout;
}

long Inflater::readInflateOutput(void *buffer, size_t count, int flushCode, int *state) {
    zStream->avail_out = count;
    zStream->next_out = static_cast<uint8_t *>(buffer);
    *state = inflate_(flushCode);
    return count - zStream->avail_out;
}

int Inflater::inflate_(int flushCode) {
    int state = zng_inflate(zStream, flushCode);

    std::string error;
    switch (state) {
    case Z_OK:
    case Z_STREAM_END:
    case Z_BUF_ERROR:
        return state;
    case Z_MEM_ERROR:
        error = "inflate: not enough memory to complete the operation, ";
        error += zStream->msg;
        zng_inflateEnd(zStream);
        throw std::ios_base::failure(error);
    case Z_DATA_ERROR:
        error = "inflate: the input data was corrupted ";
        error += zStream->msg;
        zng_inflateEnd(zStream);
        throw std::ios_base::failure(error);
    case Z_STREAM_ERROR:
        error = "inflate: the stream structure was inconsistent,";
        error += zStream->msg;
        throw std::ios_base::failure(error);
    default:
        error = "inflate: error unexpected, ";
        error += zStream->msg;
        throw std::ios_base::failure(error);
    }
}

bool Inflater::resetStreamForLeftoverInput() {
    const uint8_t *strm = zStream->next_in;
    long len = zStream->avail_in;
    if (strm == nullptr)
        return true;
    if (strm[0] != GZIP_Header_ID1 || (len > 1 && strm[1] != GZIP_Header_ID2))
        return true;

    zng_inflateEnd(zStream);
    delete zStream;
    zStream = new zng_stream{.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL};
    zng_inflateInit2(zStream, m_windowBits);
    zStream->next_in = strm;
    zStream->avail_in = len;
    m_finished = false;
    return false;
}

napi_value Inflater::JSConstructor(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO(2)
    int windowBits = -15;
    long uncompressedSize = -1;
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[0], &windowBits))
    }
    NAPI_CALL(env, napi_typeof(env, argv[1], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int64(env, argv[1], &uncompressedSize))
    }
    try {
        Inflater *inflater = new Inflater(windowBits, uncompressedSize);
        NAPI_CALL(env, napi_wrap(env, _this, inflater, JSDispose, nullptr, nullptr))
        return _this;
    } catch (const std::exception &e) {
        return nullptr;
    }
}

void Inflater::JSDispose(napi_env env, void *data, void *hint) {
    Inflater *inflater = static_cast<Inflater *>(data);
    delete inflater;
    inflater = nullptr;
}

napi_value Inflater::JSGetIsFinished(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO_WITH_INFLATER(0)
    napi_value result = nullptr;
    napi_get_boolean(env, inflater->isFinished(), &result);
    return result;
}
napi_value Inflater::JSInflate(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO_WITH_INFLATER(3)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    long readBytes = inflater->inflate(offset_pointer(buffer, offset), count);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, readBytes, &result))
    return result;
}
napi_value Inflater::JSGetIsGzipStream(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO_WITH_INFLATER(0)
    napi_value result = nullptr;
    napi_get_boolean(env, inflater->isGzipStream(), &result);
    return result;
}

napi_value Inflater::JSSetInput(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO_WITH_INFLATER(3)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    inflater->setInput(offset_pointer(buffer, offset), count);
    return nullptr;
}
napi_value Inflater::JSNeedInput(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO_WITH_INFLATER(0)
    napi_value result = nullptr;
    napi_get_boolean(env, inflater->needInput(), &result);
    return result;
}

napi_value Inflater::JSDispose(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO(0)
    void *result = nullptr;
    NAPI_CALL(env, napi_remove_wrap(env, _this, &result))
    return nullptr;
}

napi_value Inflater::JSIsDisposed(napi_env env, napi_callback_info info) {
    GET_INFLATER_INFO(0)
    void *inflater = nullptr;
    NAPI_CALL(env, napi_unwrap(env, _this, &inflater))
    bool value = inflater == nullptr;
    napi_value result = nullptr;
    napi_get_boolean(env, value, &result);
    return result;
}

void Inflater::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_FUNCTION("isFinished", nullptr, JSGetIsFinished, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("inflate", JSInflate, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isGzipInput", JSGetIsGzipStream, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("setInput", JSSetInput, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("dispose", JSDispose, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isDisposed", nullptr, JSIsDisposed, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("needInput", nullptr, JSNeedInput, nullptr, nullptr),
    };
    napi_value cons = nullptr;
    napi_define_class(env, "Inflater", NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]), desc,
                      &cons);
    napi_set_named_property(env, exports, "Inflater", cons);
}