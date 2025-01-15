//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/FileStream.h"
#include <cstdio>
#include <unistd.h>


std::string FileStream::ClassName = "FileStream";
napi_ref FileStream::cons = nullptr;

FileStream::FileStream(const std::string &path, FILE_MODE mode, long bufferSize)
    : m_file_path(path), m_mode(mode), m_bufferSize(bufferSize) {

    m_canGetLength = true;
    m_canGetPosition = true;
    m_canSeek = true;
    m_canRead = true;
    char open_mode[3] = "rb";
    if (mode & FILE_MODE_WRITE) {
        open_mode[2] = '+';
        m_canWrite = true;
    }
    if ((mode & FILE_MODE_TRUNC) || ((mode & FILE_MODE_CREATE) && (access(path.c_str(), F_OK) != 0))) {
        open_mode[0] = 'w';
    }

    FILE *fp = fopen(path.c_str(), open_mode);
    if (fp == nullptr)
        throw std::ios::failure(std::string("open file ") + path + " failed");

    fseek(fp, 0, SEEK_END);
    m_length = ftell(fp);

    if (mode & FILE_MODE_APPEND) {
        m_canRead = false;
        m_canWrite = true;
        m_canSeek = false;
        m_position = m_length;
    } else {
        fseek(fp, 0, SEEK_SET);
    }

    file = fp;
}

FileStream::~FileStream() {
    if (!m_closed) {
        IStream::close();
        close();
    }
}

long FileStream::write(void *buffer, long offset, size_t count) {
    if (m_closed)
        throw std::ios_base::failure("The write operation failed because the file was closed ");

    if (fseek(file, m_position, SEEK_SET) == -1)
        throw std::ios_base::failure(std::string("the file might have been closed. "));

    const char *pointer = static_cast<char *>(buffer) + offset;
    long writeBytes = fwrite(pointer, 1, count, file);

    if (writeBytes == -1)
        throw std::ios::failure("write stream failed");

    m_position += writeBytes;
    m_length = std::max(m_length, m_position);
    return writeBytes;
}

long FileStream::read(void *buffer, long offset, size_t count) {
    if (m_closed)
        throw std::ios_base::failure("The write operation failed because the file was closed ");
    long readBytes = count;
    readBytes = std::min(readBytes, m_length - m_position);
    if (readBytes == 0)
        return 0;

    if (fseek(file, m_position, SEEK_SET) == -1)
        throw std::ios_base::failure(std::string("the file might have been closed. "));

    char *pointer = static_cast<char *>(buffer) + offset;
    readBytes = fread(pointer, 1, readBytes, file);

    if (readBytes == -1)
        throw std::ios::failure("read stream failed");

    m_position += readBytes;
    return readBytes;
}

void FileStream::flush() {
    if (fflush(file) == -1) {
        throw std::ios::failure("flush stream failed");
    }
}

void FileStream::close() {
    if (!m_closed) {
        IStream::close();
        fclose(file);
        file = nullptr;
    }
}

void FileStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
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

void FileStream::setLength(long length) {
    if (-1 == ftruncate(fileno(file), length)) {
        throw std::ios::failure("set length failed");
    }

    m_position = std::min(length, m_position);
    m_length = length;
}
