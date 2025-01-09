//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
#include "ZipCryptoStream.h"
#include "crypt.h"
#include "zlib-ng.h"

ZipCryptoStream::ZipCryptoStream(IStream *stream, CryptoMode mode, const std::string &password, bool leaveOpen,
                                 unsigned long crc)
    : m_stream(stream), m_mode(mode), m_password(password), m_leaveOpen(leaveOpen), m_crc(crc) {
    m_canWrite = m_mode == CryptoMode::CryptoMode_Decode;
    m_canRead = m_mode == CryptoMode::CryptoMode_Encode;
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

long ZipCryptoStream::read(void *buffer, long offset, size_t count) {
    long readBytes = m_stream->read(buffer, offset, count);
    long _offset = 0;
    if (m_total_read < 12) {
        _offset = 12 - m_total_read;
    }
    m_total_read += readBytes;
    if (readBytes < _offset)
        return 0;

    readBytes -= _offset;
    uint8_t *_buffer = static_cast<uint8_t *>(buffer);
    for (long i = _offset; i < readBytes; i++) {
        zdecode(pkeys, zng_get_crc_table(), *(_buffer + offset + i));
    }
    return readBytes;
}

long ZipCryptoStream::write(void *buffer, long offset, size_t count) {
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
}