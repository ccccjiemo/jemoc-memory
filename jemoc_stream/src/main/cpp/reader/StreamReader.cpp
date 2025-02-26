//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "reader/StreamReader.h"
#include "stream/FileStream.h"

StreamReader::StreamReader(std::shared_ptr<IStream> stream, bool detectEncodingFromByteOrderMarks,
                           const std::string &encoding, bool leaveOpen)
    : stream(stream), buffer(4096), decodedBuffer(4096), bufferSize(4096), bufferPos(0), bufferLen(0), decodedPos(0),
      decodedLen(0), detectEncodingFromByteOrderMarks(detectEncodingFromByteOrderMarks), encoding(encoding),
      endOfStream(false), leaveOpen(leaveOpen) {
    if (!stream) {
        throw std::invalid_argument("stream cannot be null");
    }
    if (detectEncodingFromByteOrderMarks) {
        DetectEncoding();
    }
}

StreamReader::StreamReader(const std::string &path, bool detectEncodingFromByteOrderMarks, const std::string &encoding,
                           bool leaveOpen)
    : StreamReader(std::make_shared<FileStream>(path, FILE_MODE_READ, 4096), detectEncodingFromByteOrderMarks, encoding,
                   leaveOpen) {}

int StreamReader::Read() {
    if (endOfStream)
        return -1;
    if (decodedPos >= decodedLen) {
        if (!DecodeNextBuffer()) {
            endOfStream = true;
            return -1;
        }
    }
    return static_cast<char>(decodedBuffer[decodedPos++]);
}

int StreamReader::Peek() {
    if (endOfStream)
        return -1;
    if (decodedPos >= decodedLen) {
        if (!DecodeNextBuffer()) {
            return -1;
        }
    }
    return static_cast<char>(decodedBuffer[decodedPos]);
}

void StreamReader::Close() {
    endOfStream = true;
    if (stream) {
        if (auto fileStream = dynamic_cast<std::ifstream *>(stream.get())) {
            fileStream->close();
        }
        stream.reset();
    }
}

bool StreamReader::DecodeNextBuffer() {
    if (endOfStream)
        return false;
    if (!FillBuffer())
        {
                endOfStream = true;
    return false;
        }


    if (!converter) {
        converter = EncodingConverterFactory::Create(encoding);
        if (!converter)
            throw std::runtime_error("Unsupported encoding: " + encoding);
    }

    // 扩大解码缓冲区（每次解码至少能容纳4倍原始数据）
    std::vector<char> tempBuffer(bufferLen * 4);
    size_t convertedChars = 0;

    bool success = converter->Convert(buffer.data() + bufferPos, bufferLen - bufferPos, tempBuffer.data(),
                                      tempBuffer.size(), convertedChars);


    if (success && convertedChars > 0) {
        decodedBuffer.assign(tempBuffer.begin(), tempBuffer.begin() + convertedChars);
        bufferPos = bufferLen; // 标记缓冲区已消费
        decodedPos = 0;
        decodedLen = convertedChars;
        return true;
    }

    endOfStream = true;
    return false;
}

bool StreamReader::FillBuffer() {
    if (!stream || stream->isClose()) {
        return false;
    }

    if (bufferPos > 0 && bufferLen > bufferPos) {
        std::copy(buffer.begin() + bufferPos, buffer.begin() + bufferLen, buffer.begin());
        bufferLen -= bufferPos;
    } else {
        bufferLen = 0;
    }
    bufferPos = 0;

    size_t remainingSpace = bufferSize - bufferLen;
    if (remainingSpace > 0) {
        size_t readBytes = stream->read(buffer.data() + bufferLen, bufferLen, remainingSpace);
        bufferLen += readBytes;
    }

    return bufferLen > 0;
}

void StreamReader::DetectEncoding() {
    if (stream->isClose()) {
        return;
    }

    unsigned char bom[4] = {0};
    auto readCount = stream->read(reinterpret_cast<char *>(bom), 0, 4);

    size_t skipBytes = 0;
    if (readCount >= 2) {
        uint16_t bom16 = (bom[0] << 8) | bom[1];

        switch (bom16) {
        case 0xFEFF: // UTF-16BE
            encoding = "UTF-16BE";
            skipBytes = 2;
            break;

        case 0xFFFE: // UTF-16LE 或 UTF-32LE
            if (readCount >= 4 && bom[2] == 0x00 && bom[3] == 0x00) {
                encoding = "UTF-32LE";
                skipBytes = 4;
            } else {
                encoding = "UTF-16LE";
                skipBytes = 2;
            }
            break;

        case 0xEFBB: // 可能是 UTF-8
            if (readCount >= 3 && bom[2] == 0xBF) {
                encoding = "UTF-8";
                skipBytes = 3;
            }
            break;

        case 0x0000: // 可能是 UTF-32BE
            if (readCount >= 4 && bom[2] == 0xFE && bom[3] == 0xFF) {
                encoding = "UTF-32BE";
                skipBytes = 4;
            }
            break;
        }
    }

    if (stream->getCanSeek()) {
        stream->seek(skipBytes - readCount, Current);
    } else {
        bufferLen = readCount - skipBytes;
        if (bufferLen > 0) {
            std::copy(bom + skipBytes, bom + readCount, buffer.begin());
        }
        bufferPos = 0;
    }
}
