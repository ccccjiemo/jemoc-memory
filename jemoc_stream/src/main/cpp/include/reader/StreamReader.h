//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_STREAMREADER_H
#define JEMOC_STREAM_TEST_STREAMREADER_H

#include "IStream.h"
#include "memory"
#include "reader/EncodingConverter.h"
#include "reader/TextReader.h"

class StreamReader : public TextReader {
private:
    std::shared_ptr<IStream> stream;
    std::vector<char> buffer;
    std::vector<char> decodedBuffer;
    size_t bufferSize;
    size_t bufferPos;
    size_t bufferLen;
    size_t decodedPos;
    size_t decodedLen;
    bool detectEncodingFromByteOrderMarks;
    std::string encoding;
    std::unique_ptr<EncodingConverter> converter;
    bool endOfStream;
    bool leaveOpen;

    bool DecodeNextBuffer();
    bool FillBuffer();
    void DetectEncoding();

public:
    explicit StreamReader(std::shared_ptr<IStream> stream, bool detectEncodingFromByteOrderMarks = true,
                          const std::string &encoding = "UTF-8", bool leaveOpen = false);

    explicit StreamReader(const std::string &path, bool detectEncodingFromByteOrderMarks = true,
                          const std::string &encoding = "UTF-8", bool leaveOpen = false);

    int Read() override;
    int Peek() override;
    void Close() override;
    const std::string &GetEncoding() const { return encoding; }

    bool EndOfStream() const { return endOfStream; }
};

#endif // JEMOC_STREAM_TEST_STREAMREADER_H
