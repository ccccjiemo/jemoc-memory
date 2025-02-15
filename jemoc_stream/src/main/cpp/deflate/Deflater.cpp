//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "deflate/Deflater.h"
#include "common.h"
#include "zip/ZipArchiveEntry.h"
#include <ios>

static int getMemLevel(int level) { return level == Z_NO_COMPRESSION ? 7 : 8; }

Deflater::Deflater(int windowBits, int level, int strategy)
    : m_windowBits(windowBits), m_level(level), m_strategy(strategy) {
    if (windowBits < Min_WINDOW_BITS || windowBits > Max_WINDOW_BITS)
        throw std::ios_base::failure("deflater: windowbits must be greater than -15 and less than 31. ");
    zStream = new zng_stream{.zalloc = Z_NULL, .zfree = Z_NULL, .opaque = Z_NULL};
    int errCode = zng_deflateInit2(zStream, m_level, Z_DEFLATED, m_windowBits, getMemLevel(m_level), m_strategy);

    if (errCode == Z_OK)
        return;

    std::string err;
    err = "deflater: deflateInit2 failed";
    err += zStream->msg;

    delete zStream;
    zStream = nullptr;

    throw std::ios_base::failure(err);
}

Deflater::~Deflater() {
    if (zStream != nullptr) {
        zng_deflateEnd(zStream);
        delete zStream;
        zStream = nullptr;
    }
}

bool Deflater::needInput() const { return zStream->avail_in == 0; }
void Deflater::setInput(void *buffer, size_t count) {
    mtx.lock();
    zStream->avail_in = count;
    zStream->next_in = static_cast<const uint8_t *>(buffer);
    mtx.unlock();
}

long Deflater::getDeflateOutput(void *buffer, size_t count) {
    size_t bytesRead = 0;
    readDeflateOutput(buffer, count, Z_NO_FLUSH, &bytesRead);
    return bytesRead;
}

int Deflater::readDeflateOutput(void *buffer, size_t count, int flushCode, size_t *bytesRead) {
    mtx.lock();
    zStream->next_out = static_cast<uint8_t *>(buffer);
    zStream->avail_out = count;
    int errCode = deflate(flushCode);
    *bytesRead = count - zStream->avail_out;
    mtx.unlock();
    return errCode;
}

int Deflater::deflate(int flushCode) {
    int errCode = zng_deflate(zStream, flushCode);

    switch (errCode) {
    case Z_OK:
    case Z_STREAM_END:
        return errCode;
    case Z_BUF_ERROR:
        return errCode;
    case Z_STREAM_ERROR:
        throw std::ios_base::failure("deflater: deflate failed, inconsistent stream ");
    default:
        throw std::ios_base::failure(std::string("deflater: deflate failed, ") + zStream->msg);
    }
}

bool Deflater::finish(void *buffer, size_t count, size_t *bytesRead) {
    int errCode = readDeflateOutput(buffer, count, Z_FINISH, bytesRead);
    return errCode == Z_STREAM_END;
}

bool Deflater::flush(void *buffer, size_t count, size_t *bytesRead) {
    return readDeflateOutput(buffer, count, Z_SYNC_FLUSH, bytesRead) == Z_OK;
}


napi_value Deflater::JSConstructor(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO(3)
    int windowBits = -15;
    int level = Z_DEFAULT_COMPRESSION;
    int strategy = Z_DEFAULT_STRATEGY;
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[0], &windowBits))
    }
    NAPI_CALL(env, napi_typeof(env, argv[1], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[1], &level))
    }
    NAPI_CALL(env, napi_typeof(env, argv[2], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[1], &strategy))
    }
    try {
        Deflater *deflater = new Deflater(windowBits, level, strategy);
        NAPI_CALL(env, napi_wrap(env, _this, deflater, JSDispose, nullptr, nullptr))
        return _this;
    } catch (const std::exception &e) {
        return nullptr;
    }
}

void Deflater::JSDispose(napi_env env, void *data, void *hint) {
    Deflater *deflater = static_cast<Deflater *>(data);
    delete deflater;
    deflater = nullptr;
}

napi_value Deflater::JSSetInput(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO_WITH_DEFLATER(3)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    deflater->setInput(offset_pointer(buffer, offset), count);
    return nullptr;
}
napi_value Deflater::JSNeedInput(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO_WITH_DEFLATER(0)
    napi_value result = nullptr;
    napi_get_boolean(env, deflater->needInput(), &result);
    return result;
}

napi_value Deflater::JSDispose(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO(0)
    void *result = nullptr;
    NAPI_CALL(env, napi_remove_wrap(env, _this, &result))
    return nullptr;
}

napi_value Deflater::JSIsDisposed(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO(0)
    void *deflater = nullptr;
    NAPI_CALL(env, napi_unwrap(env, _this, &deflater))
    bool value = deflater == nullptr;
    napi_value result = nullptr;
    napi_get_boolean(env, value, &result);
    return result;
}

napi_value Deflater::JSFlush(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO_WITH_DEFLATER(4)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    size_t bytesRead = 0;
    bool success = deflater->flush(offset_pointer(buffer, offset), count, &bytesRead);
    napi_value values[2]{nullptr};
    NAPI_CALL(env, napi_get_boolean(env, success, &values[0]))
    NAPI_CALL(env, napi_create_int64(env, bytesRead, &values[1]))
    napi_property_descriptor desc[] = {
        {"result", nullptr, nullptr, nullptr, nullptr, argv[0], napi_default, nullptr},
        {"readBytes", nullptr, nullptr, nullptr, nullptr, argv[1], napi_default, nullptr},
    };
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object_with_properties(env, &result, 2, desc))
    return result;
}

napi_value Deflater::JSFinish(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO_WITH_DEFLATER(4)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    size_t bytesRead = 0;
    bool success = deflater->finish(offset_pointer(buffer, offset), count, &bytesRead);
    napi_value values[2]{nullptr};
    NAPI_CALL(env, napi_get_boolean(env, success, &values[0]))
    NAPI_CALL(env, napi_create_int64(env, bytesRead, &values[1]))
    napi_property_descriptor desc[] = {
        {"result", nullptr, nullptr, nullptr, nullptr, argv[0], napi_default, nullptr},
        {"readBytes", nullptr, nullptr, nullptr, nullptr, argv[1], napi_default, nullptr},
    };
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object_with_properties(env, &result, 2, desc))
    return result;
}

napi_value Deflater::JSDeflate(napi_env env, napi_callback_info info) {
    GET_DEFLATER_INFO_WITH_DEFLATER(4)
    void *buffer = nullptr;
    size_t length = 0;
    getBuffer(env, argv[0], &buffer, &length);
    long offset = getOffset(env, argv[1], length);
    size_t count = getCount(env, argv[2], length, offset);
    long readBytes = deflater->getDeflateOutput(offset_pointer(buffer, offset), count);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, readBytes, &result))
    return result;
}

void Deflater::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_FUNCTION("setInput", JSSetInput, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("needInput", nullptr, JSNeedInput, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("dispose", JSDispose, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isDisposed", nullptr, JSIsDisposed, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("flush", JSFlush, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("finish", JSFinish, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("deflate", JSDeflate, nullptr, nullptr, nullptr),
    };
    napi_value cons = nullptr;
    NAPI_CALL(env, napi_define_class(env, "Deflater", NAPI_AUTO_LENGTH, JSConstructor, nullptr,
                                     sizeof(desc) / sizeof(desc[0]), desc, &cons))
    NAPI_CALL(env, napi_set_named_property(env, exports, "Deflater", cons))
}