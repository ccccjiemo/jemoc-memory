//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/MemoryStream.h"
#include "common.h"

std::string MemoryStream::ClassName = "MemoryStream";

MemoryStream::MemoryStream() {
    setCapacity(1);
    m_canWrite = true;
    m_canSeek = true;
    m_canRead = true;
    m_canGetPosition = true;
    m_canGetLength = true;
}

MemoryStream::MemoryStream(size_t capacity) {
    setCapacity(capacity);
    m_canWrite = true;
    m_canSeek = true;
    m_canRead = true;
    m_canGetPosition = true;
    m_canGetLength = true;
}


MemoryStream::~MemoryStream() { close(); }

void MemoryStream::setLength(long length) {
    ensureCapacity(length);
    m_length = length;
    m_position = std::min(m_position, length);
}

long MemoryStream::read(void *buffer, long offset, size_t count) {
    if (count == 0)
        return 0;
    size_t readBytes = m_length - m_position;
    readBytes = std::min(readBytes, count);
    memcpy(offset_pointer(buffer, offset), m_cache.data() + m_position, readBytes);
    m_position += readBytes;
    return readBytes;
}

long MemoryStream::write(void *buffer, long offset, size_t count) {
    if (count == 0)
        return 0;
    ensureCapacity(m_position + count);
    void *dest = m_cache.data() + m_position;
    void *source = offset_pointer(buffer, offset);
    memcpy(dest, source, count);
    m_position += count;
    m_length = std::max(m_position, m_length);
    return count;
}

void MemoryStream::ensureCapacity(long capacity) {
    if (capacity > m_length && capacity > m_capacity) {
        long newCapacity = std::max(capacity, 256L);
        if (newCapacity < m_capacity * 2) {
            newCapacity = m_capacity * 2;
        }
        setCapacity(newCapacity);
    }
}

void MemoryStream::setCapacity(long capacity) {
    m_capacity = capacity;
    m_cache.resize(m_capacity);
}
long MemoryStream::getCapacity() const { return m_capacity; }

void MemoryStream::close() {
    if (m_closed)
        return;
    IStream::close();
    m_cache.clear();
    m_cache.resize(0);
}


napi_ref MemoryStream::cons = nullptr;

/**
 * MemoryStream构造函数，入参可能是个number或者是个arraybuffer
 * @param env
 * @param info
 * @return
 */
napi_value MemoryStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(1);
    napi_valuetype type;
    napi_typeof(env, argv[0], &type);
    MemoryStream *stream = new MemoryStream();
    if (type == napi_number) {
        long capacity = getLong(env, argv[0]);
        stream->setCapacity(capacity);
    } else {
        void *data = nullptr;
        size_t length = 0;
        getBuffer(env, argv[0], &data, &length);
        if (data != nullptr) {
            stream->setCapacity(length);
            stream->write(data, 0, length);
        }
    }

    napi_wrap(env, _this, stream, JSDisposed, nullptr, nullptr);
    return _this;
}

void MemoryStream::JSDisposed(napi_env env, void *data, void *hint) {
    MemoryStream *stream = static_cast<MemoryStream *>(data);
    stream->close();
    delete stream;
}

napi_value MemoryStream::JSToArrayBuffer(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    void *data = nullptr;
    napi_value buffer = nullptr;
    long length = stream->getLength();
    napi_create_arraybuffer(env, length, &data, &buffer);
    uint8_t* _buffer = static_cast<uint8_t*>(data) ;
    memcpy(data, static_cast<MemoryStream *>(stream)->m_cache.data(), length);
    return buffer;
}

napi_value MemoryStream::JSGetCapacity(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    RETURN_NAPI_VALUE(napi_create_int64, static_cast<MemoryStream *>(stream)->getCapacity());
}
napi_value MemoryStream::JSSetCapacity(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    long capacity = getLong(env, argv[0]);
    if (capacity < 0 || capacity < stream->getPosition()) {
        napi_throw_range_error(env, "MemoryStream::setCapacity", "capacity is out of range");
    }
    static_cast<MemoryStream *>(stream)->setCapacity(capacity);
    return nullptr;
}


void MemoryStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
        DEFINE_NAPI_FUNCTION("capacity", nullptr, JSGetCapacity, JSSetCapacity, nullptr),
        DEFINE_NAPI_FUNCTION("toArrayBuffer", JSToArrayBuffer, nullptr, nullptr, nullptr),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);

    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}
