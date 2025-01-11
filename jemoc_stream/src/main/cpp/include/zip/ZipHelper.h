//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPHELPERE_H
#define JEMOC_STREAM_TEST_ZIPHELPERE_H

#include "IStream.h"
#include <sys/types.h>
namespace ZipHelper {
/**
 * 当流读取到开头返回true
 * @param stream
 * @param buffer
 * @param bufferSize
 * @param bufferPointer
 * @return
 */
bool seekBackwardsAndRead(IStream *stream, void *buffer, size_t bufferSize, long *bufferPointer) {
    if (stream->getPosition() >= bufferSize) {
        stream->seek(-bufferSize, SeekOrigin::Current);
        stream->read(buffer, 0, bufferSize);
        stream->seek(-bufferSize, SeekOrigin::Current);
        *bufferPointer = bufferSize - 1;
        return false;
    } else {
        long bytesToRead = stream->getPosition();
        stream->seek(0, SeekOrigin::Begin);
        stream->read(buffer, 0, bytesToRead);
        stream->seek(0, SeekOrigin::Begin);
        *bufferPointer = bytesToRead - 1;
        return true;
    }
};
/**
 * 回退查找Signature
 * @param stream
 * @param signature
 * @param maxBytesToRead
 * @return
 */
bool seekBackwardsToSignature(IStream *stream, uint signature, long maxBytesToRead) {
    bool outOfBytes = false;
    bool signatureFound = false;
    uint currentSignature = 0;
    uint8_t *buffer = new uint8_t[32];

    long bufferPointer = 0;
    long bytesRead = 0;
    do {
        outOfBytes = seekBackwardsAndRead(stream, buffer, 32, &bufferPointer);
        while (bufferPointer >= 0 && !signatureFound) {
            currentSignature = (currentSignature << 8) | ((uint)buffer[bufferPointer]);
            if (currentSignature == signature) {
                signatureFound = true;
            } else {
                bufferPointer--;
            }
        }
        bytesRead += 32;
    } while (!signatureFound && !outOfBytes && bytesRead <= maxBytesToRead);

    if (!signatureFound) {
        return false;
    } else {
        stream->seek(bufferPointer, SeekOrigin::Current);
        return true;
    }
}
}; // namespace ZipHelper

#endif // JEMOC_STREAM_TEST_ZIPHELPERE_H
