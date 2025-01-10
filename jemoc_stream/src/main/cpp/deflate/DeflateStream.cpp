//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "DeflateStream.h"

napi_ref DeflateStream::cons = nullptr;
std::string DeflateStream::ClassName = "DeflateStream";
DeflateStream::DeflateStream(IStream *stream, DeflateMode mode, int windowBits, int compressionLevel, bool leaveOpen,
                             size_t bufferSize, long uncompressSize)
    : m_stream(stream), m_mode(mode), m_windowBits(windowBits), m_compressionLevel(compressionLevel),
      m_leaveOpen(leaveOpen), m_uncompressSize(uncompressSize), m_bufferSize(bufferSize) {

    m_canSeek = false;
    m_canGetLength = false;
    m_length = 0;
    m_position = 0;
    m_canGetPosition = false;
    m_buffer = malloc(m_bufferSize);

    if (m_buffer == nullptr) {
        throw std::ios::failure("DeflateStream: failed to allocate buffer cache.");
    }
    switch (mode) {
    case DeflateMode_Compress:
        if (!stream->getCanWrite()) {
            throw std::ios_base::failure("DeflateStream: The target stream is not writable.");
        }
        m_canWrite = true;
        deflater = new Deflater(m_windowBits, m_compressionLevel, Z_DEFAULT_STRATEGY);

        break;
    case DeflateMode_Decompress:
        if (!stream->getCanRead()) {
            throw std::ios_base::failure("DeflateStream: The target stream is not readable.");
        }
        inflater = new Inflater(m_windowBits, m_uncompressSize);
        m_canRead = true;
        break;
    default:
        throw std::ios_base::failure("DeflateStream: the DeflateMode is out of range.");
    }
}

DeflateStream::~DeflateStream() { close(); }

bool DeflateStream::inflaterIsFinished() const {
    return inflater != nullptr && inflater->isFinished() && (!inflater->isGzipStream() || !inflater->needInput());
}

long DeflateStream::getPosition() const { throw std::ios_base::failure("DeflateStream: get position not supported."); }
long DeflateStream::getLength() const { throw std::ios_base::failure("DeflateStream: get length not supported."); }
long DeflateStream::seek(long offset, SeekOrigin origin) {
    throw std::ios_base::failure("DeflateStream: seek operation not supported.");
}
void DeflateStream::close() {
    if (m_closed)
        return;
    IStream::close();
    purgeBuffers();
    if (m_buffer != nullptr) {
        free(m_buffer);
        m_buffer = nullptr;
    }
    if (inflater != nullptr) {
        delete inflater;
        inflater = nullptr;
    }
    if (deflater != nullptr) {
        delete deflater;
        deflater = nullptr;
    }
    if (!m_leaveOpen && m_stream != nullptr) {
        m_stream->close();
        m_stream = nullptr;
    }
}

void DeflateStream::flush() {
    if (m_closed)
        throw std::ios::failure("DeflateStream: stream is closed.");
    if (m_mode == DeflateMode_Compress) {
        flushBuffers();
    }
}

long DeflateStream::read(void *buffer, long offset, size_t count) {
    if (m_mode != DeflateMode::DeflateMode_Decompress)
        throw std::ios_base::failure("DeflateStream: decompress mode does not support read operation.");
    if (inflater == nullptr)
        throw std::ios_base::failure("DeflateStream: inflater is null");
    if (m_closed)
        throw std::ios::failure("DeflateStream: stream is closed");
    size_t bytesRead = 0;
    if (buffer == nullptr)
        return bytesRead;
    while (true) {
        bytesRead = inflater->inflate(offset_pointer(buffer, offset), count);
        if (bytesRead != 0 || inflaterIsFinished()) {
            break;
        }
        if (inflater->needInput()) {
            size_t n = m_stream->read(m_buffer, 0, m_bufferSize);
            if (n <= 0) {
                if (!inflater->isFinished()) {
                    throw std::ios::failure("DeflateStream: found truncated data while decoding.");
                }
                break;
            } else {
                inflater->setInput(m_buffer, n);
            }
        }
    }
    return bytesRead;
}
long DeflateStream::write(void *buffer, long offset, size_t count) {
    if (m_mode != DeflateMode::DeflateMode_Compress)
        throw std::ios_base::failure("DeflateStream: decompress mode does not support read operation.");
    if (m_closed)
        throw std::ios::failure("DeflateStream: stream is closed.");
    if (buffer == nullptr)
        return 0;
    if (deflater == nullptr)
        throw std::ios::failure("DeflateStream: deflater is null ");

    deflater->setInput(offset_pointer(buffer, offset), count);
    writeDeflaterOutput();
    m_wroteBytes = true;
    return count;
}

void DeflateStream::writeDeflaterOutput() {
    while (!deflater->needInput()) {
        long compressedBytes = deflater->getDeflateOutput(m_buffer, m_bufferSize);
        if (compressedBytes > 0) {
            m_stream->write(m_buffer, 0, compressedBytes);
        }
    }
}

void DeflateStream::flushBuffers() {
    if (m_wroteBytes) {
        writeDeflaterOutput();
        bool success;
        do {
            size_t compressedBytes = 0;
            success = deflater->flush(m_buffer, m_bufferSize, &compressedBytes);
            if (success) {
                m_stream->write(m_buffer, 0, compressedBytes);
            }
        } while (success);
    }
    m_stream->flush();
}

void DeflateStream::purgeBuffers() {
    if (m_stream == nullptr)
        return;
    if (m_mode != DeflateMode::DeflateMode_Compress)
        return;

    bool finished;

    if (m_wroteBytes) {
        writeDeflaterOutput();
        do {
            size_t compressedBytes = 0;
            finished = deflater->finish(m_buffer, m_bufferSize, &compressedBytes);
            if (compressedBytes > 0) {
                m_stream->write(m_buffer, 0, compressedBytes);
            }
        } while (!finished);
    } else {
        do {
            size_t s;
            finished = deflater->finish(m_buffer, m_bufferSize, &s);
        } while (!finished);
    }
}

napi_value DeflateStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(3)
    IStream *stream = getStream(env, argv[0]);
    if (stream == nullptr)
        napi_throw_error(env, "DeflateStream", "argument stream is null");

    int mode = getInt(env, argv[1]);
    bool leaveOpen = false;
    int windowBits = -15;
    int compressionLevel = Z_DEFAULT_COMPRESSION;
    long bufferSize = DEFAULT_BUFFER_SIZE;
    long uncompressSize = -1;

    napi_value value = nullptr;
    napi_valuetype type;
    GET_OBJ(argv[2], "leaveOpen", napi_get_value_bool, leaveOpen)
    GET_OBJ(argv[2], "windowBits", napi_get_value_int32, windowBits)
    GET_OBJ(argv[2], "uncompressSize", napi_get_value_int64, uncompressSize)
    GET_OBJ(argv[2], "bufferSize", napi_get_value_int64, bufferSize)
    GET_OBJ(argv[2], "compressionLevel", napi_get_value_int32, compressionLevel)

    if (windowBits < Min_WINDOW_BITS || windowBits > Max_WINDOW_BITS)
        napi_throw_error(env, "DeflateStream", "windowBits must be greater than -15 and less than 31.");
    if (mode == DeflateMode_Decompress && uncompressSize < -1)
        napi_throw_range_error(env, "DeflateStream", "uncompressSize must greater than -1 in decompress mode");

    if (bufferSize < 1)
        napi_throw_range_error(env, ClassName.c_str(), "bufferSize must greater than 1");

    try {
        DeflateStream *ds = new DeflateStream(stream, DeflateMode(mode), windowBits, compressionLevel, leaveOpen,
                                              bufferSize, uncompressSize);
        napi_wrap(env, _this, ds, JSDispose, nullptr, nullptr);
    } catch (const std::ios::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }

    return _this;
}

void DeflateStream::JSDispose(napi_env env, void *data, void *hint) {
    DeflateStream *stream = static_cast<DeflateStream *>(data);
    delete stream;
    stream = nullptr;
}

void DeflateStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);
    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}
