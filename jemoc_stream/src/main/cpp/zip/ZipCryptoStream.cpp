//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#include "zip/ZipCryptoStream.h"
#include "stream/MemoryStream.h"
#include "zip/ZipArchive.h"
#include "zip/ZipArchiveEntry.h"
#include <cstdint>


std::string ZipCryptoStream::ClassName = "ZipCryptoStream";
napi_ref ZipCryptoStream::cons = nullptr;

ZipCryptoStream::ZipCryptoStream(std::shared_ptr<IStream> stream, CryptoMode mode, const std::string &password,
                                 bool leaveOpen, unsigned long crc, size_t bufferSize)
    : m_stream(stream), m_mode(mode), m_password(password), m_leaveOpen(leaveOpen), m_crc(crc),
      m_buffer_Size(bufferSize), m_everRead(false), m_everWrite(false) {
    m_canWrite = m_mode == CryptoMode::CryptoMode_Encode;
    m_canRead = m_mode == CryptoMode::CryptoMode_Decode;
    m_canSeek = false;

    if (stream == nullptr)
        throw std::ios::failure("stream is null");
    if (m_password.empty())
        throw std::ios::failure("password is empty");

    init_keys(m_password.c_str(), pkeys, zng_get_crc_table());
}

ZipCryptoStream::ZipCryptoStream(std::shared_ptr<IStream> stream, CryptoMode mode, ZipArchiveEntry *entry,
                                 bool leaveOpen, size_t bufferSize)
    : m_stream(stream), m_mode(mode), m_entry(entry), m_leaveOpen(leaveOpen), m_buffer_Size(bufferSize),
      m_everRead(false), m_everWrite(false) {
    m_canWrite = m_mode == CryptoMode::CryptoMode_Encode;
    m_canRead = m_mode == CryptoMode::CryptoMode_Decode;
    m_canSeek = false;

    if (stream == nullptr)
        throw std::ios::failure("stream is null");
    m_password = entry->getArchive()->getPassword();
    if (m_password.empty())
        throw std::ios::failure("password is empty");
    m_crc = entry->getCryptCRC();
    init_keys(m_password.c_str(), pkeys, zng_get_crc_table());
}

ZipCryptoStream::~ZipCryptoStream() { close(); }


void ZipCryptoStream::writeCrypthead() {
    unsigned char buffer[RAND_HEAD_LEN];
    crypthead(m_password.c_str(), buffer, RAND_HEAD_LEN, pkeys, zng_get_crc_table(), m_crc);
    m_stream->write(buffer, 0, RAND_HEAD_LEN);
}

void ZipCryptoStream::ensureNotClose() {
    if (m_closed)
        throw std::ios::failure("stream is closed");
}

void ZipCryptoStream::checkStream() {
    if (m_stream == nullptr || m_stream->isClose())
        throw std::ios::failure("innerStream is closed");
}
void ZipCryptoStream::ensureBuffer() {
    if (m_buffer != nullptr)
        return;
    m_buffer = (uint8_t *)malloc(sizeof(uint8_t) * m_buffer_Size);
    if (m_buffer == nullptr)
        throw std::ios::failure("malloc buffer failed");
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


    if (!this->m_everRead) {
        uint8_t *buffer = new uint8_t[12];
        if (m_stream->read(buffer, 0, 12) != 12) {
            delete[] buffer;
            throw std::ios::failure("read crypto head less than 12.");
        }
        decodeStream(buffer, 0, 12);
        uint crc = m_crc;
        if (m_entry != nullptr) {
            crc = m_entry->getCryptCRC();
        }
        if ((((crc >> 16) & 0xff) != buffer[10]) || (((crc >> 24) & 0xff) != buffer[11])) {
            throw std::ios::failure("invalid password");
        }
        this->m_everRead = true;
    }

    size_t bytesRead = m_stream->read(buffer, offset, count);
    decodeStream(buffer, offset, bytesRead);
    m_total_read += bytesRead;

    return bytesRead;
}

long ZipCryptoStream::write(void *buffer, long offset, size_t count) {
    ensureNotClose();
    checkStream();
    ensureBuffer();

//     if (m_write_finished) {
//         int t;
//         const uint32_t *crc_tab = zng_get_crc_table();
//         long p = 0;
//         uint8_t *_buffer = static_cast<uint8_t *>(buffer);
//
//
//         for (long i = 0; i < count; i++) {
//             zencode(pkeys, crc_tab, *(_buffer + offset + i), t);
//             *(m_buffer + p) = t;
//             p += 1;
//             if (m_buffer_Size == p) {
//                 m_stream->write(m_buffer, 0, m_buffer_Size);
//                 p = 0;
//             }
//         }
//         if (p > 0) {
//             m_stream->write(m_buffer, 0, p);
//         }
//         return count;
//     } else {
//         if (m_cache == nullptr) {
//             m_cache = new MemoryStream();
//         }
//         m_cache->write(buffer, offset, count);
//     }
//     return count;
    if (!m_everWrite) {
        m_everWrite = true;
        if (m_entry != nullptr) {
            m_crc = m_entry->getCryptCRC();
        }

        writeCrypthead();
    }
    size_t chunkSize = 0;
    size_t totalWrite = 0;
    while (totalWrite != count) {
        chunkSize = std::min(count - totalWrite, m_buffer_Size);
        memcpy(m_buffer, offset_pointer(buffer, offset + totalWrite), chunkSize);
        encodeStream(m_buffer, 0, chunkSize);
        m_stream->write(m_buffer, 0, chunkSize);
        totalWrite += chunkSize;
    }

    return count;
}


void ZipCryptoStream::close() {
    if (m_closed)
        return;
    IStream::close();
    if (!m_leaveOpen) {
        m_stream->close();
        m_stream.reset();
    }
    m_stream = nullptr;
    if (m_buffer != nullptr) {
        free(m_buffer);
        m_buffer = nullptr;
    }
}

long ZipCryptoStream::getPosition() const { throw std::ios::failure("ZipCryptoStream: not support get position"); }
long ZipCryptoStream::getLength() const { throw std::ios::failure("ZipCryptoStream: not support get length."); }

napi_value ZipCryptoStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(5)
    std::shared_ptr<IStream> stream = IStream::GetStream(env, argv[0]);

    if (stream == nullptr)
        napi_throw_error(env, "ZipCryptoStream", "argument stream is null");
    int mode = getInt(env, argv[1]);
    std::string passwd = getString(env, argv[2]);
    unsigned char crc = getLong(env, argv[3]);
    long bufferSize = 8192;
    bool leaveOpen = false;
    napi_value value = nullptr;
    napi_valuetype type;
    GET_OBJ(argv[4], "bufferSize", napi_get_value_int64, bufferSize);
    GET_OBJ(argv[4], "leaveOpen", napi_get_value_bool, leaveOpen);

    try {
        std::shared_ptr<IStream> cryptoStream =
            std::make_shared<ZipCryptoStream>(stream, CryptoMode(mode), passwd, leaveOpen, crc, bufferSize);
//        ZipCryptoStream *cryptoStream =
//            new ZipCryptoStream(stream, CryptoMode(mode), passwd, leaveOpen, crc, bufferSize);
        if (napi_ok != napi_wrap(env, _this, new SharedPtrWrapper(cryptoStream), JSDispose, nullptr, nullptr))
            throw std::ios::failure("napi_wrap failed");
    } catch (const std::ios::failure &e) {
        napi_throw_error(env, "ZipCryptoStream", e.what());
        return nullptr;
    }

    return _this;
}

void ZipCryptoStream::JSDispose(napi_env env, void *data, void *hint) {
//    ZipCryptoStream *stream = static_cast<ZipCryptoStream *>(data);
//    stream->close();
    SharedPtrWrapper *wrapper = static_cast<SharedPtrWrapper *>(data);
    delete wrapper;
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