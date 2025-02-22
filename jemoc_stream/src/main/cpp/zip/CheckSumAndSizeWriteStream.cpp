//
// Created on 2025/1/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/CheckSumAndSizeWriteStream.h"
#include "zlib-ng.h"
#include <cstdint>

CheckSumAndSizeWriteStream::CheckSumAndSizeWriteStream(std::shared_ptr<IStream> stream,
                                                       std::shared_ptr<IStream> baseStream, bool leaveOpen,
                                                       std::function<void(long, long, uint)> onClose) {
    if (stream == nullptr)
        throw std::ios::failure("stream is null");
    m_canSeek = false;
    m_canRead = false;
    m_canWrite = true;
    m_canGetLength = true;
    m_canGetPosition = false;
    m_stream = stream;
    m_position = 0;
    m_onClose = onClose;
    m_leaveOpen = leaveOpen;
    m_baseStream = baseStream;
}


long CheckSumAndSizeWriteStream::write(void *buffer, long offset, size_t count) {
    if (count == 0)
        return 0;
    if (!m_everWritten) {
        m_initialPosition = m_baseStream->getPosition();
        m_everWritten = true;
    }
    m_checksum = zng_crc32_z(m_checksum, static_cast<uint8_t *>(buffer) + offset, count);
    m_stream->write(buffer, offset, count);
    m_position += count;
    return count;
}

void CheckSumAndSizeWriteStream::flush() { m_stream->flush(); }

void CheckSumAndSizeWriteStream::close() {
    if (m_closed)
        return;
    IStream::close();
    if (!m_everWritten)
        m_initialPosition = m_baseStream->getPosition();
    if (!m_leaveOpen) {
        m_stream->close();
        m_stream.reset();
        m_stream = nullptr;
    }
    m_baseStream.reset();
    m_baseStream = nullptr;
    m_onClose(m_initialPosition, m_position, m_checksum);
}