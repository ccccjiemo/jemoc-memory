//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#include "ZipCryptoStream.h"


std::string ZipCryptoStream::ClassName = "ZipCryptoStream";
napi_ref ZipCryptoStream::cons = nullptr;

ZipCryptoStream::ZipCryptoStream(IStream *stream, CryptoMode mode, const std::string &password, bool leaveOpen,
                                 unsigned long crc, size_t bufferSize)
    : m_stream(stream), m_mode(mode), m_password(password), m_leaveOpen(leaveOpen), m_crc(crc),
      m_buffer_Size(bufferSize) {
    m_canWrite = m_mode == CryptoMode::CryptoMode_Encode;
    m_canRead = m_mode == CryptoMode::CryptoMode_Decode;
    m_canSeek = false;

    if (stream == nullptr)
        throw std::ios::failure("ZipCryptoStream: stream is null");
    if (m_password.empty())
        throw std::ios::failure("ZipCryptoStream: password is empty");

    init_keys(m_password.c_str(), pkeys, zng_get_crc_table());

    if (m_crc != 0 && mode == CryptoMode_Encode) {
        m_hasCrc = true;
        writeCrypthead();
    }
}

void ZipCryptoStream::writeCrypthead() {
    unsigned char buffer[RAND_HEAD_LEN];
    crypthead(m_password.c_str(), buffer, RAND_HEAD_LEN, pkeys, zng_get_crc_table(), m_crc);
    m_stream->write(buffer, 0, RAND_HEAD_LEN);
}

void ZipCryptoStream::ensureNotClose() {
    if (m_closed)
        throw std::ios::failure("ZipCryptoStream: stream is closed");
}

void ZipCryptoStream::checkStream() {
    if (m_stream == nullptr || m_stream->isClose())
        throw std::ios::failure("ZipCryptoStream: innerStream is closed");
}
void ZipCryptoStream::ensureBuffer() {
    if (m_buffer != nullptr)
        return;
    m_buffer = (uint8_t *)malloc(sizeof(uint8_t) * m_buffer_Size);
    if (m_buffer == nullptr)
        throw std::ios::failure("ZipCryptoStream: malloc buffer failed");
}

void ZipCryptoStream::decodeStream(void *buffer, long offset, size_t count) {
    const uint32_t *crc_tab = zng_get_crc_table();
    for (long i = 0; i < count; i++) {
        zdecode(pkeys, crc_tab, *((uint8_t *)buffer + offset + i));
    }
}

void ZipCryptoStream::encodeStream(void *buffer, long offset, size_t count) {
    int t;
    const uint32_t *crc_tab = zng_get_crc_table();
    for (long i = 0; i < count; i++) {
        *((uint8_t *)buffer + offset + i) = zencode(pkeys, crc_tab, *((uint8_t *)buffer + offset + i), t);
    }
}


long ZipCryptoStream::read(void *buffer, long offset, size_t count) {
    ensureNotClose();
    checkStream();

    size_t needRead = 0;
    size_t bytesRead = 0;
    if (m_total_read < 12) {
        needRead = 12 - m_total_read;
        uint8_t *_buffer = new uint8_t[needRead];
        bytesRead = m_stream->read(_buffer, 0, needRead);
        if (bytesRead == 0)
            return 0;
        decodeStream(_buffer, 0, bytesRead);
        m_total_read += bytesRead;
        if (bytesRead != needRead) {
            delete[] _buffer;
            return 0;
        } else {
            uint32_t verify1 = *(_buffer + needRead - 2);
            uint32_t verify2 = *(_buffer + needRead - 1);
            delete[] _buffer;
        }
    }

    bytesRead = m_stream->read(buffer, offset, count);
    decodeStream(buffer, offset, bytesRead);
    m_total_read += bytesRead;

    return bytesRead;
}

long ZipCryptoStream::write(void *buffer, long offset, size_t count) {
    ensureNotClose();
    checkStream();
    ensureBuffer();
    int t;
    const uint32_t *crc_tab = zng_get_crc_table();
    long p = 0;
    uint8_t *_buffer = static_cast<uint8_t *>(buffer);


    for (long i = 0; i < count; i++) {
        zencode(pkeys, crc_tab, *(_buffer + offset + i), t);
        *(m_buffer + p) = t;
        p += 1;
        if (m_buffer_Size == p) {
            m_stream->write(m_buffer, 0, m_buffer_Size);
            p = 0;
        }
    }
    if (p > 0) {
        m_stream->write(m_buffer, 0, p);
    }
    return count;
}

void ZipCryptoStream::close() {
    if (m_closed)
        return;
    IStream::close();
    if (!m_leaveOpen) {
        m_stream->close();
    }
    m_stream = nullptr;
    if (m_buffer != nullptr) {
        free(m_buffer);
    }
}

long ZipCryptoStream::getPosition() const { throw std::ios::failure("ZipCryptoStream: not support get position"); }
long ZipCryptoStream::getLength() const { throw std::ios::failure("ZipCryptoStream: not support get length."); }

napi_value ZipCryptoStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(5)
    IStream *stream = getStream(env, argv[0]);

    if (stream == nullptr)
        napi_throw_error(env, "ZipCryptoStream", "argument stream is null");
    int mode = getInt(env, argv[1]);
    std::string passwd = getString(env, argv[2]);
    unsigned char crc = getLong(env, argv[3]);
    long bufferSize = 4096;
    bool leaveOpen = false;
    napi_value value = nullptr;
    napi_valuetype type;
    GET_OBJ(argv[4], "bufferSize", napi_get_value_int64, bufferSize);
    GET_OBJ(argv[4], "leaveOpen", napi_get_value_bool, leaveOpen);

    try {
        ZipCryptoStream *cryptoStream =
            new ZipCryptoStream(stream, CryptoMode(mode), passwd, leaveOpen, crc, bufferSize);
        if (napi_ok != napi_wrap(env, _this, cryptoStream, JSDispose, nullptr, nullptr))
            throw std::ios::failure("napi_wrap failed");
    } catch (const std::ios::failure &e) {
        napi_throw_error(env, "ZipCryptoStream", e.what());
        return nullptr;
    }

    return _this;
}

void ZipCryptoStream::JSDispose(napi_env env, void *data, void *hint) {
    ZipCryptoStream *stream = static_cast<ZipCryptoStream *>(data);
    stream->close();
}

void ZipCryptoStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);
    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}