//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "FileStream.h"

std::string FileStream::ClassName = "FileStream";
napi_ref FileStream::cons = nullptr;

FileStream::FileStream(const std::string &path, FILE_MODE mode, long bufferSize)
    : m_file_path(path), m_mode(mode), m_bufferSize(bufferSize) {
    m_fileStream.open(path, m_mode);

    if (!m_fileStream.is_open()) {
        m_fileStream.close();
        throw std::ios_base::failure("FileStream: open file failed");
    }

    m_fileStream.exceptions(std::ios::failbit | std::ios::badbit);
    m_canSeek = (m_mode & FILE_MODE_APPEND) != 0;
    m_canRead = (m_mode & FILE_MODE_READ) != 0;
    m_canWrite = (m_mode & FILE_MODE_WRITE) != 0;

    m_fileStream.seekg(0, std::ios_base::end);
    m_length = m_fileStream.tellg();
    m_position = m_canSeek ? 0 : m_length;
    m_position = (m_mode & FILE_MODE_ATE) != 0 ? m_length : m_position;
    m_canGetLength = true;
    m_closed = false;
}

FileStream::~FileStream() {
    if (!m_closed) {
        IStream::close();
        close();
    }
}

long FileStream::write(void *buffer, long offset, size_t count) {
    if (!m_fileStream.is_open()) {
        close();
        throw std::ios_base::failure("FileStream: The write operation failed because the file was closed ");
    }
    try {
        m_fileStream.seekp(m_position, std::ios_base::beg);
        const char *pointer = static_cast<char *>(buffer) + offset;
        m_fileStream.write(pointer, count);
    } catch (const std::ios::ios_base::failure &e) {
        close();
        std::string err = ClassName += ": ";
        err += e.what();
        throw std::ios_base::failure(err);
    }
    m_position += count;
    m_length = std::max(m_length, m_position);
    return count;
}

long FileStream::read(void *buffer, long offset, size_t count) {
    if (!m_fileStream.is_open()) {
        close();
        throw std::ios_base::failure("FileStream: The rad operation failed because the file was closed ");
    }
    long readBytes = count;
    readBytes = std::min(readBytes, m_length - m_position);
    if (readBytes == 0)
        return 0;
    try {
        m_fileStream.seekg(m_position, std::ios_base::beg);
        char *pointer = static_cast<char *>(buffer) + offset;
        m_fileStream.read(pointer, readBytes);
    } catch (const std::ios::ios_base::failure &e) {
        close();
        std::string err = ClassName += ": ";
        err += e.what();
        throw std::ios_base::failure(err);
    }
    m_position += readBytes;
    return readBytes;
}

void FileStream::flush() { m_fileStream.flush(); }

void FileStream::close() {
    if (!m_closed) {
        IStream::close();
        m_fileStream.close();
    }
}

void FileStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void*)ClassName.c_str()),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);

    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}

napi_value FileStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(2);

    std::string path = getString(env, argv[0]);
    int mode = getInt(env, argv[1]);
    FileStream *stream = nullptr;
    try {
        stream = new FileStream(path, FILE_MODE(mode), 4096);
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "JSFileStream:", e.what());
        return nullptr;
    }

    napi_wrap(env, _this, stream, JSDispose, nullptr, nullptr);
    return _this;
}

void FileStream::JSDispose(napi_env env, void *data, void *hint) {
    FileStream *stream = static_cast<FileStream *>(data);
    stream->close();
    delete stream;
}