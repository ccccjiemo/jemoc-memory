//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/MemoryStream.h"
#include "common.h"

std::string MemoryStream::ClassName = "MemoryStream";

MemoryStream::MemoryStream() {
//     m_cache = new std::vector<byte>();
    setCapacity(1);
    m_canWrite = true;
    m_canSeek = true;
    m_canRead = true;
    m_canGetPosition = true;
    m_canGetLength = true;
    m_canSetLength = true;
}

MemoryStream::MemoryStream(size_t capacity) {
//     m_cache = new std::vector<byte>();
    setCapacity(capacity);
    m_canWrite = true;
    m_canSeek = true;
    m_canRead = true;
    m_canGetPosition = true;
    m_canGetLength = true;
    m_canSetLength = true;
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
//     memcpy(offset_pointer(buffer, offset), m_cache->data() + m_position, readBytes);
    memcpy(offset_pointer(buffer, offset), mm_cache + m_position, readBytes);

    m_position += readBytes;
    return readBytes;
}

long MemoryStream::write(void *buffer, long offset, size_t count) {
    if (count == 0)
        return 0;
    ensureCapacity(m_position + count);
//     void *dest = m_cache->data() + m_position;
    void *dest = mm_cache + m_position;

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
//     m_capacity = capacity;
//     m_cache->resize(m_capacity);
    m_capacity = align4k(capacity);
    byte *buffer = new byte[m_capacity];

    if (mm_cache == nullptr) {
        mm_cache = buffer;
        return;
    }

    byte *temp = mm_cache;
    memcpy(buffer, temp, std::min(m_length, capacity));
    mm_cache = buffer;
    delete[] temp;
    temp = nullptr;
}
long MemoryStream::getCapacity() const { return m_capacity; }

void MemoryStream::close() {
    if (m_closed)
        return;
    IStream::close();
//     m_cache.clear();
//     m_cache.erase(m_cache.begin(), m_cache.end());
//     m_cache.resize(0);
//     m_cache->erase(m_cache->begin(), m_cache->end());
//     m_cache->shrink_to_fit();
//     m_cache->resize(0);
//     delete m_cache;
//     m_cache = nullptr;
    if (mm_cache != nullptr) {
        delete[] mm_cache;
        mm_cache = nullptr;
    }
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
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
//    MemoryStream *stream = new MemoryStream();
    std::shared_ptr<IStream> stream = std::make_shared<MemoryStream>();
    if (type != napi_undefined) {
        if (type == napi_number) {
            long capacity = getLong(env, argv[0]);
            ((MemoryStream *)stream.get())->setCapacity(capacity);
            stream = std::make_shared<MemoryStream>(capacity);
        } else {
            void *data = nullptr;
            size_t length = 0;
            getBuffer(env, argv[0], &data, &length);
            if (data != nullptr) {
                stream->write(data, 0, length);
            }
        }
    }
//    napi_wrap(env, _this, MakePtr(stream), JSDisposed, nullptr, nullptr);
//    return _this;
    return JSBind(env, _this, stream);
}

//void MemoryStream::JSDisposed(napi_env env, void *data, void *hint) {
//    MemoryStream *stream = static_cast<MemoryStream *>(data);
//    stream->close();
//    delete stream;
//}

napi_value MemoryStream::JSToArrayBuffer(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    void *data = nullptr;
    napi_value buffer = nullptr;
    long length = stream->getLength();
    long offset = 0;
    if (argc > 0) {
        getToArrayBufferOptions(env, argv[0], &offset, &length);
    }
    NAPI_CALL(env, napi_create_arraybuffer(env, length, &data, &buffer));

    memcpy(data, ((MemoryStream *)stream.get())->getData() + offset, length);
    return buffer;
}

napi_value MemoryStream::JSGetCapacity(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    RETURN_NAPI_VALUE(napi_create_int64, static_cast<MemoryStream *>(stream.get())->getCapacity());
}
napi_value MemoryStream::JSSetCapacity(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    long capacity = getLong(env, argv[0]);
    if (capacity < 0 || capacity < stream->getPosition()) {
        napi_throw_range_error(env, "MemoryStream::setCapacity", "capacity is out of range");
    }
    static_cast<MemoryStream *>(stream.get())->setCapacity(capacity);
    return nullptr;
}


void MemoryStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
//        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
        DEFINE_NAPI_FUNCTION("capacity", nullptr, JSGetCapacity, JSSetCapacity, nullptr),
        DEFINE_NAPI_FUNCTION("toArrayBuffer", JSToArrayBuffer, nullptr, nullptr, nullptr),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
//    napi_create_reference(env, napi_cons, 1, &cons);
    Extends(env, napi_cons);

    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}
