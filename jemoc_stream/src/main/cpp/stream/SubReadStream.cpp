//
// Created on 2025/1/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/SubReadStream.h"

SubReadStream::SubReadStream(std::shared_ptr<IStream> stream, long startPosition, size_t maxLength, bool leaveOpen)
    : m_stream(stream), m_startInStream(startPosition), m_endInStream(startPosition + maxLength),
      m_leaveOpen(leaveOpen) {
    m_position = 0;
    m_canRead = true;
    m_canWrite = false;
    m_canSeek = false;
}

SubReadStream::~SubReadStream() { close(); }

bool SubReadStream::getCanRead() const {
    if (auto stream = m_stream.lock()) {
        return stream->getCanRead() && m_canRead;
    } else {
        return false;
    }
}

long SubReadStream::read(void *buffer, long offset, size_t count) {

    if (auto stream = m_stream.lock()) {
        if ((m_position + m_startInStream) != stream->getPosition()) {
            stream->seek(m_position + m_startInStream, SeekOrigin::Begin);
        }
        size_t _count = std::min((size_t)(m_endInStream - m_startInStream - m_position), count);
        long result = stream->read(buffer, offset, _count);
        m_position += result;
        return result;
    } else {
        throw std::ios::failure("stream is closed");
    }
}

void SubReadStream::close() {
    if (m_closed)
        return;
    IStream::close();
    auto stream = m_stream.lock();
    if (!m_leaveOpen && stream) {
        stream->close();
        m_stream.reset();
    }
}