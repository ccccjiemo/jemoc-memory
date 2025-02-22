//
// Created on 2025/2/17.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_BROTLISTREAM_H
#define JEMOC_STREAM_TEST_BROTLISTREAM_H

#include "IStream.h"
#include "brotli/decode.h"
#include "brotli/encode.h"


enum OperationStatus {
    OperationStatus_Done,
    OperationStatus_DestinationTooSmall,
    OperationStatus_NeedMoreData,
    OperationStatus_InvalidData,
};

struct Buffer {
    uint8_t *ptr;
    size_t length;

    Buffer(void *buffer, long offset, size_t count) {
        ptr = static_cast<uint8_t *>(buffer) + offset;
        length = count;
    }

    void slice(size_t offset) {
        ptr += offset;
        length -= offset;
    }
};

struct BrotliConfig {
    int quality = 6;
    int lgWin = 22;
    int mode = 0;
    int lgBlock = 0;
    bool largeWindow = false;
};

static void getBrotliConfig(napi_env env, napi_value value, BrotliConfig &config) {
    napi_valuetype type;
    napi_value jsVal;
    NAPI_CALL(env, napi_get_named_property(env, value, "quality", &jsVal))
    NAPI_CALL(env, napi_typeof(env, jsVal, &type))
    if (type == napi_number) {
        config.quality = std::max(BROTLI_MIN_QUALITY, std::min(BROTLI_MAX_QUALITY, getInt(env, jsVal)));
    }
    NAPI_CALL(env, napi_get_named_property(env, value, "lgWin", &jsVal))
    NAPI_CALL(env, napi_typeof(env, jsVal, &type))
    if (type == napi_number) {
        config.lgWin = std::max(BROTLI_MIN_WINDOW_BITS, std::min(BROTLI_MAX_WINDOW_BITS, getInt(env, jsVal)));
    }
    NAPI_CALL(env, napi_get_named_property(env, value, "mode", &jsVal))
    NAPI_CALL(env, napi_typeof(env, jsVal, &type))
    if (type == napi_number) {
        config.mode = std::max(0, std::min(2, getInt(env, jsVal)));
    }
}

class BrotliDecoder;
class BrotliEncoder;


class BrotliStream : public IStream {
public:
    enum CompressionMode { Compress, Decompress };


public:
    BrotliStream(std::shared_ptr<IStream> stream, CompressionMode compressionMode, const BrotliConfig &config, bool leaveOpen,
                 size_t bufferSize);
    ~BrotliStream();

    long read(void *buffer, long offset, size_t length) override;
    long write(void *buffer, long offset, size_t length) override;
    void flush() override;
    void close() override;

public:
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static std::string ClassName;
//    static void JSDispose(napi_env env, void *data, void *hint);
    static void Export(napi_env env, napi_value exports);
//    napi_ref stream_ref = nullptr;


private:
    long writeCore(void *buffer, long offset, size_t length, bool isFinalBlock = false);
    bool tryDecompress(Buffer destination, size_t &bytesWritten);

private:
    BrotliDecoder *m_decoder;
    BrotliEncoder *m_encoder;
    std::shared_ptr<IStream> m_stream;
    CompressionMode m_mode;
    uint8_t *buffer_;
    long bufferOffset_;
    size_t bufferCount_;
    size_t bufferSize_;
    bool nonEmptyInput_ = false;
    bool m_leaveOpen;
};

class BrotliDecoder {
public:
    BrotliDecoder();
    ~BrotliDecoder();

public:
    OperationStatus decompress(Buffer source, Buffer destination, size_t &bytesConsumed, size_t &bytesWritten);
    static bool tryDecompress(Buffer &source, Buffer &destination, size_t &bytesWritten);
    static std::vector<uint8_t> tryDecompress(const uint8_t *input, size_t input_size);

public:
private:
    BrotliDecoderState *m_decoder;
};

class BrotliEncoder {
public:
    BrotliEncoder(const BrotliConfig &config);
    ~BrotliEncoder();

public:
    OperationStatus compress(Buffer &source, Buffer &destination, size_t &bytesConsumed, size_t &bytesWritten,
                             bool isFinalBlock);
    OperationStatus compress(Buffer &source, Buffer &destination, size_t &bytesConsumed, size_t &bytesWritten,
                             BrotliEncoderOperation operation);

    OperationStatus flush(Buffer &destination, size_t &bytesWritten);

    static bool tryCompress(const Buffer source, Buffer destination, size_t &bytesWritten, const BrotliConfig &config);

    static size_t getMaxCompressedLength(size_t inputSize);

private:
    BrotliEncoderState *m_encoder;
};

class BrotliJs {
    struct AsyncData {
        napi_async_work work;
        napi_deferred deferred;
        std::pair<std::shared_ptr<uint8_t>, size_t> buffer;
        napi_value result;
        BrotliConfig config;
    };

public:
    static napi_value JSDecompress(napi_env env, napi_callback_info info);
    static napi_value JSCompress(napi_env env, napi_callback_info info);
    static void Export(napi_env env, napi_value exports);
    static napi_value JSDecompressCore(napi_env env, void *buffer, size_t length);
    static napi_value JSCompressCore(napi_env env, void *buffer, size_t length, const BrotliConfig &config);
    static std::pair<std::shared_ptr<uint8_t>, size_t> GetBuffer(napi_env env, napi_value value);
    static napi_value JSDecompressAsync(napi_env env, napi_callback_info info);
    static napi_value JSCompressAsync(napi_env env, napi_callback_info info);
};


#endif // JEMOC_STREAM_TEST_BROTLISTREAM_H
