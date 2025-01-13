//
// Created on 2025/1/13.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
#define JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
#include "IStream.h"

class WrappedStream : public IStream {
public:
    WrappedStream(IStream *stream, bool leaveOpen, std::function<void()> onClose)
        : m_stream(stream), m_leaveOpen(leaveOpen), m_onClose(onClose) {}
    ~WrappedStream() { close(); }

    bool getCanRead() const override { return !m_closed && m_stream->getCanRead(); }
    bool getCanWrite() const override { return !m_closed && m_stream->getCanWrite(); }
    long read(void *buffer, long offset, size_t count) override {
        if (getCanRead()) {
            return m_stream->read(buffer, offset, count);
        }
        return 0;
    }
    long write(void *buffer, long offset, size_t count) override {
        if (getCanWrite()) {
            m_stream->write(buffer, offset, count);
            return count;
        }
        return 0;
    }
    void close() override {
        if (m_closed)
            return;
        IStream::close();
        if (m_onClose) {
            m_onClose();
        }
        if (!m_leaveOpen) {
            m_stream->close();
        }
        m_stream = nullptr;
        delete this;
    }

private:
    IStream *m_stream = nullptr;
    bool m_leaveOpen = false;
    std::function<void()> m_onClose;
};

#endif // JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
