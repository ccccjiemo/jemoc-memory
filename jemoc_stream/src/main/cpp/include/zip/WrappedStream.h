//
// Created on 2025/1/13.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
#define JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
#include "IStream.h"
#include "zip/ZipArchiveEntry.h"

class WrappedStream : public IStream {
public:
    WrappedStream(std::shared_ptr<IStream> stream, ZipArchiveEntry *entry, bool leaveOpen, std::function<void()> onClose)
        : m_stream(stream), m_leaveOpen(leaveOpen), m_onClose(onClose), m_entry(entry) {}
    ~WrappedStream() { close(); }
    bool getCanSeek() const override { return !m_closed && m_stream->getCanSeek(); }
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
        m_entry = nullptr;
        m_stream = nullptr;
    }
    long getPosition() const override { return m_stream->getPosition(); }
    long getLength() const override { return m_stream->getLength(); }
    void setLength(long length) override { m_stream->setLength(length); }
    void copyTo(IStream *stream, long bufferSize) override { m_stream->copyTo(stream, bufferSize); }
    long seek(long offset, SeekOrigin origin) override { return m_stream->seek(offset, origin); }

private:
    ZipArchiveEntry *m_entry = nullptr;
    std::shared_ptr<IStream> m_stream = nullptr;
    bool m_leaveOpen = false;
    std::function<void()> m_onClose;
};

#endif // JEMOC_STREAM_TEST_WRAPPEDSTREAM_H
