//
// Created on 2025/2/18.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/BrotliStream.h"


BrotliEncoder::BrotliEncoder(const BrotliConfig &config) : m_encoder(nullptr) {
    m_encoder = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
}
BrotliEncoder::~BrotliEncoder() {
    if (m_encoder != nullptr) {
        BrotliEncoderDestroyInstance(m_encoder);
        m_encoder = nullptr;
    }
}


OperationStatus BrotliEncoder::compress(Buffer &source, Buffer &destination, size_t &bytesConsumed,
                                        size_t &bytesWritten, bool isFinalBlock) {
    return compress(source, destination, bytesConsumed, bytesWritten,
                    isFinalBlock ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS);
}

OperationStatus BrotliEncoder::compress(Buffer &source, Buffer &destination, size_t &bytesConsumed,
                                        size_t &bytesWritten, BrotliEncoderOperation operation) {
    bytesWritten = 0;
    bytesConsumed = 0;
    size_t availableOutput = destination.length;
    size_t availableInput = source.length;

    while (availableOutput > 0) {
        const uint8_t *input = source.ptr;
        uint8_t *output = destination.ptr;

        if (BrotliEncoderCompressStream(m_encoder, operation, &availableInput, &input, &availableOutput, &output,
                                        nullptr) == BROTLI_FALSE) {
            return OperationStatus_InvalidData;
        }
        bytesConsumed += source.length - availableInput;
        bytesWritten += destination.length - availableOutput;

        if (availableOutput == destination.length && BrotliEncoderHasMoreOutput(m_encoder) == BROTLI_FALSE &&
            availableInput == 0) {
            return OperationStatus_Done;
        }
        source.slice(source.length - availableInput);
        destination.slice(destination.length - availableOutput);
    }

    return OperationStatus_DestinationTooSmall;
}

OperationStatus BrotliEncoder::flush(Buffer &destination, size_t &bytesWritten) {
    size_t bytesConsumed = 0;
    Buffer emptyBuffer(nullptr, 0, 0);
    return compress(emptyBuffer, destination, bytesConsumed, bytesWritten, BROTLI_OPERATION_FLUSH);
}

bool BrotliEncoder::tryCompress(const Buffer source, Buffer destination, size_t &bytesWritten,
                                const BrotliConfig &config) {
    const uint8_t *input = source.ptr;
    uint8_t *output = destination.ptr;
    size_t availableOutput = destination.length;
    bool success = BrotliEncoderCompress(config.quality, config.lgWin, BrotliEncoderMode(config.mode), source.length,
                                         input, &availableOutput, output) != BROTLI_FALSE;

    bytesWritten = availableOutput;
    return success;
}

size_t BrotliEncoder::getMaxCompressedLength(size_t inputSize) { return BrotliEncoderMaxCompressedSize(inputSize); }

