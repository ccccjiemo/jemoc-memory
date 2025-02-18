//
// Created on 2025/2/18.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/BrotliStream.h"

BrotliDecoder::BrotliDecoder() : m_decoder(nullptr) {
    m_decoder = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
}

BrotliDecoder::~BrotliDecoder() {
    if (m_decoder != nullptr) {
        BrotliDecoderDestroyInstance(m_decoder);
        m_decoder = nullptr;
    }
}

OperationStatus BrotliDecoder::decompress(Buffer source, Buffer destination, size_t &bytesConsumed,
                                          size_t &bytesWritten) {
    bytesConsumed = 0;
    bytesWritten = 0;

    if (BrotliDecoderIsFinished(m_decoder) != BROTLI_FALSE) {
        return OperationStatus_Done;
    }
    size_t availableOutput = destination.length;
    size_t availableInput = source.length;
    while (availableOutput > 0) {
        const uint8_t *input = source.ptr;
        uint8_t *output = destination.ptr;
        BrotliDecoderResult result =
            BrotliDecoderDecompressStream(m_decoder, &availableInput, &input, &availableOutput, &output, nullptr);
        if (result == BrotliDecoderResult::BROTLI_DECODER_RESULT_ERROR) {
            return OperationStatus_InvalidData;
        }
        bytesConsumed += source.length - availableInput;
        bytesWritten += destination.length - availableOutput;

        switch (result) {
        case BROTLI_DECODER_RESULT_SUCCESS:
            return OperationStatus_Done;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
            return OperationStatus_DestinationTooSmall;
        case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
        default:
            source.slice(source.length - availableInput);
            destination.slice(destination.length - availableOutput);
            if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT && source.length == 0)
                return OperationStatus_NeedMoreData;
            break;
        }
    }
    return OperationStatus_DestinationTooSmall;
}

bool BrotliDecoder::tryDecompress(Buffer &source, Buffer &destination, size_t &bytesWritten) {
    const uint8_t *input = source.ptr;
    uint8_t *output = destination.ptr;
    size_t availableOutput = destination.length;

    bool success = BrotliDecoderDecompress(source.length, input, &availableOutput, output) ==
                   BrotliDecoderResult::BROTLI_DECODER_RESULT_SUCCESS;
    bytesWritten = availableOutput;
    return success;
}


std::vector<uint8_t> BrotliDecoder::tryDecompress(const uint8_t *input, size_t input_size) {
    std::vector<uint8_t> output;
    BrotliDecoderState *state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!state) {
        OH_LOG_ERROR(LOG_APP, "Failed to create Brotli decoder instance");
        return output;
    }

    size_t available_in = input_size;
    const uint8_t *next_in = input;
    size_t total_out = 0;

    BrotliDecoderResult result;
    do {
        size_t available_out = 0;
        uint8_t *next_out = nullptr;
        result = BrotliDecoderDecompressStream(state, &available_in, &next_in, &available_out, &next_out, &total_out);

        if (result == BROTLI_DECODER_RESULT_ERROR) {
            OH_LOG_ERROR(LOG_APP, "Decoding error: %s", BrotliDecoderErrorString(BrotliDecoderGetErrorCode(state)));
            break;
        }

        const uint8_t *output_buffer = BrotliDecoderTakeOutput(state, &available_out);
        if (available_out > 0) {
            output.insert(output.end(), output_buffer, output_buffer + available_out);
        }
    } while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT);

    BrotliDecoderDestroyInstance(state);

    if (result != BROTLI_DECODER_RESULT_SUCCESS) {
        output.clear();
    }

    return output;
}






