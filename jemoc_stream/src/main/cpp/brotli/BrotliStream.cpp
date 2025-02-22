//
// Created on 2025/2/18.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/BrotliStream.h"

std::string BrotliStream::ClassName = "BrotliStream";

BrotliStream::BrotliStream(std::shared_ptr<IStream> stream, CompressionMode compressionMode, const BrotliConfig &config,
                           bool leaveOpen, size_t bufferSize)
    : m_stream(stream), m_mode(compressionMode), m_decoder(nullptr), m_encoder(nullptr), m_leaveOpen(leaveOpen),
      bufferSize_(bufferSize) {

    m_canGetPosition = false;
    m_canSeek = false;
    m_canGetLength = false;
    m_canSetLength = false;

    m_canRead = compressionMode == CompressionMode::Decompress;
    m_canWrite = compressionMode == CompressionMode::Compress;

    buffer_ = new uint8_t[bufferSize_];
    bufferCount_ = 0;
    bufferOffset_ = 0;

    if (compressionMode == CompressionMode::Compress) {
        m_encoder = new BrotliEncoder(config);
    } else {
        m_decoder = new BrotliDecoder();
    }
}

BrotliStream::~BrotliStream() { close(); }

long BrotliStream::read(void *buffer, long offset, size_t length) {
    if (m_mode != CompressionMode::Decompress)
        throw std::ios_base::failure("BrotliStream: decompress mode does not support read operation.");
    if (m_decoder == nullptr)
        throw std::ios_base::failure("BrotliStream: decoder is null");
    if (m_closed)
        throw std::ios::failure("BrotliStream: stream is closed");

    size_t bytesWritten = 0;
    while (!tryDecompress(Buffer(buffer, offset, length), bytesWritten)) {
        size_t bytesRead = m_stream->read(buffer_, bufferCount_, bufferSize_ - bufferCount_);
        if (bytesRead <= 0) {
            break;
        }
        nonEmptyInput_ = true;
        bufferCount_ += bytesRead;
        if (bufferCount_ > bufferSize_) {
            throw std::ios_base::failure("BrotliStream: invlaid stream? buffer overflow.");
        }
    }
    return bytesWritten;
}

long BrotliStream::write(void *buffer, long offset, size_t length) { return writeCore(buffer, offset, length); }

long BrotliStream::writeCore(void *buffer, long offset, size_t length, bool isFinalBlock) {
    if (m_mode != CompressionMode::Compress)
        throw std::ios_base::failure("BrotliStream: decompress mode does not support read operation.");
    if (m_encoder == nullptr)
        throw std::ios::failure("BrotliStream: deflater is null ");

    Buffer input(buffer, offset, length);
    OperationStatus lastResult = OperationStatus_DestinationTooSmall;
    long total_write = 0;
    while (lastResult == OperationStatus_DestinationTooSmall) {
        Buffer output(buffer_, 0, bufferSize_);

        size_t bytesConsumed = 0;
        size_t bytesWritten = 0;

        lastResult = m_encoder->compress(input, output, bytesConsumed, bytesWritten, isFinalBlock);

        if (lastResult == OperationStatus_InvalidData) {
            throw std::ios_base::failure("BrotliStream: invalid data");
        }
        if (bytesWritten > 0) {
            m_stream->write(buffer_, 0, bytesWritten);
            total_write += bytesWritten;
        }
        if (bytesConsumed > 0) {
            input.slice(bytesConsumed);
        }
    }
    return total_write;
}

bool BrotliStream::tryDecompress(Buffer destination, size_t &bytesWritten) {
    Buffer source(buffer_, bufferOffset_, bufferCount_);

    size_t bytesConsumed = 0;
    OperationStatus lastResult = m_decoder->decompress(source, destination, bytesConsumed, bytesWritten);

    if (lastResult == OperationStatus_InvalidData) {
        throw std::ios_base::failure("BrotliStream: invalid data");
    }

    if (bytesConsumed != 0) {
        bufferOffset_ += bytesConsumed;
        bufferCount_ -= bytesConsumed;
    }

    if (bytesWritten != 0 || lastResult == OperationStatus_Done) {
        return true;
    }

    if (destination.length == 0 && bufferCount_ != 0) {
        return true;
    }

    if (bufferCount_ != 0 && bufferOffset_ != 0) {
        memcpy(buffer_, buffer_ + bufferOffset_, bufferCount_);
    }
    bufferOffset_ = 0;
    return false;
}

void BrotliStream::flush() {
    if (m_closed)
        throw std::ios::failure("BrotliStream: stream is closed.");

    if (m_mode == CompressionMode::Compress) {
        OperationStatus lastResult = OperationStatus_DestinationTooSmall;
        Buffer output(buffer_, 0, bufferSize_);
        while (lastResult == OperationStatus_DestinationTooSmall) {
            size_t bytesWritten = 0;
            lastResult = m_encoder->flush(output, bytesWritten);
            if (bytesWritten > 0) {
                m_stream->write(buffer_, 0, bytesWritten);
            }
        }

        m_stream->flush();
    }
}

void BrotliStream::close() {
    if (m_closed)
        return;
    IStream::close();
    if (m_encoder != nullptr) {
        if (m_mode == CompressionMode::Compress)
            writeCore(nullptr, 0, 0, true);
        delete m_encoder;
        m_encoder = nullptr;
    }
    if (m_decoder != nullptr) {
        delete m_decoder;
        m_decoder = nullptr;
    }
    if (buffer_ != nullptr) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
    if (!m_leaveOpen)
        m_stream->close();
}

void BrotliStream::Export(napi_env env, napi_value exports) {
    napi_value cons;
    NAPI_CALL(env,
              napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, 0, nullptr, &cons))
    Extends(env, cons);
    NAPI_CALL(env, napi_set_named_property(env, exports, ClassName.c_str(), cons))
}

napi_value BrotliStream::JSConstructor(napi_env env, napi_callback_info info) {
    napi_value _this;
    size_t argc = 3;
    napi_value argv[3]{nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &_this, nullptr))
    std::shared_ptr<IStream> stream = IStream::GetStream(env, argv[0]);
    if (!stream) {
        napi_throw_type_error(env, "BrotliStream", "invalid stream");
        return nullptr;
    }
    int mode = getInt(env, argv[1]);
    int quality = BROTLI_DEFAULT_QUALITY;
    int lgWin = BROTLI_DEFAULT_WINDOW;
    int b_mode = BROTLI_DEFAULT_MODE;
    bool leaveOpen = false;
    size_t bufferSize = 1024 * 8;
    if (argc == 3) {
        napi_value val;
        napi_valuetype type;
        NAPI_CALL(env, napi_get_named_property(env, argv[2], "quality", &val))
        NAPI_CALL(env, napi_typeof(env, val, &type))
        if (type == napi_number) {
            quality = getInt(env, val);
        }
        NAPI_CALL(env, napi_get_named_property(env, argv[2], "lgWin", &val))
        NAPI_CALL(env, napi_typeof(env, val, &type))
        if (type == napi_number) {
            lgWin = getInt(env, val);
        }
        NAPI_CALL(env, napi_get_named_property(env, argv[2], "mode", &val))
        NAPI_CALL(env, napi_typeof(env, val, &type))
        if (type == napi_number) {
            b_mode = getInt(env, val);
        }
        NAPI_CALL(env, napi_get_named_property(env, argv[2], "leaveOpen", &val))
        NAPI_CALL(env, napi_typeof(env, val, &type))
        if (type == napi_boolean) {
            NAPI_CALL(env, napi_get_value_bool(env, val, &leaveOpen))
        }
        NAPI_CALL(env, napi_get_named_property(env, argv[2], "bufferSize", &val))
        NAPI_CALL(env, napi_typeof(env, val, &type))
        if (type == napi_number) {
            bufferSize = getInt(env, val);
        }
    }
    BrotliConfig config{.quality = quality, .lgWin = lgWin, .mode = b_mode};
    std::shared_ptr<IStream> bs =
        std::make_shared<BrotliStream>(stream, BrotliStream::CompressionMode(mode), config, leaveOpen, bufferSize);


    return JSBind(env, _this, bs);
}

// void BrotliStream::JSDispose(napi_env env, void *data, void *hint) {
//     BrotliStream *bs = static_cast<BrotliStream *>(data);
//     bs->close();
//     if (!bs->m_leaveOpen) {
//         napi_value result;
//         void *stream = nullptr;
//         ((IStream *)stream)->close();
//         NAPI_CALL(env, napi_get_reference_value(env, bs->stream_ref, &result))
//         NAPI_CALL(env, napi_remove_wrap(env, result, &stream))
//     }
//     delete bs;
// }