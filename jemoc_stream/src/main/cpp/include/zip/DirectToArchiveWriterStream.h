//
// Created on 2025/1/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_DIRECTTOARCHIVEWRITERSTREAM_H
#define JEMOC_STREAM_TEST_DIRECTTOARCHIVEWRITERSTREAM_H
#include "IStream.h"
#include "zip/ZipArchive.h"
#include "zip/ZipArchiveEntry.h"

class DirectToArchiveWriterStream : public IStream {
public:
    DirectToArchiveWriterStream(IStream *stream, ZipArchiveEntry *entry) : m_stream(stream), m_entry(entry) {
        m_position = 0;
        m_everWritten = false;
        m_canWrite = true;
    }

    long write(void *buffer, long offset, size_t count) override {
        if (count == 0)
            return 0;
        if (!m_everWritten) {
            m_everWritten = true;
            m_entry->writeLocalFileHeader();
        }
        m_stream->write(buffer, offset, count);
        m_position += count;
    }

    void close() override {
        if (m_closed)
            return;
        IStream::close();
        m_stream->close();
        if (!m_everWritten) {
            m_entry->writeLocalFileHeader();
        } else {
            if (m_entry->getArchive()->getArchiveStream()->getCanSeek()) {
                m_entry->writeCrcAndSizesInLocalHeader();
            } else {
                m_entry->writeDataDescriptor();
            }
        }
    }

private:
    IStream *m_stream;
    ZipArchiveEntry *m_entry;
    bool m_everWritten;
};


#endif // JEMOC_STREAM_TEST_DIRECTTOARCHIVEWRITERSTREAM_H
