//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/FileStream.h"
#include <cstdio>
#include <unistd.h>

#define DEFAULT_BUFFER_SIZE 8192


std::string FileStream::ClassName = "FileStream";
napi_ref FileStream::cons = nullptr;

FileStream::FileStream(const std::string &path, FILE_MODE mode, long bufferSize)
    : m_mode(mode), m_bufferSize(bufferSize) {

    m_canGetLength = true;
    m_canGetPosition = true;
    m_canSeek = true;
    m_canRead = true;
    std::string open_mode = "rb";
    if (mode & FILE_MODE_WRITE) {
        open_mode += "+";
        m_canWrite = true;
    }
    if (mode & FILE_MODE_TRUNC) {
        open_mode[0] = 'w';
    } else if (mode & FILE_MODE_CREATE) {
        // 使用更安全的fopen替代access检查
        open_mode[0] = 'w';
    }

    FILE *fp = fopen(path.c_str(), open_mode.c_str());
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

FileStream::FileStream(const int &fd, FILE_MODE mode, long bufferSize) : m_mode(mode), m_bufferSize(bufferSize) {

    m_canGetLength = true;
    m_canGetPosition = true;
    m_canSeek = true;
    m_canRead = true;
    char open_mode[3] = "rb";
    if (mode & FILE_MODE_WRITE) {
        open_mode[2] = '+';
        m_canWrite = true;
    }
    if ((mode & FILE_MODE_TRUNC) || ((mode & FILE_MODE_CREATE))) {
        open_mode[0] = 'w';
        m_canSetLength = true;
    }

    FILE *fp = fdopen(fd, open_mode);
    if (fp == nullptr)
        throw std::ios::failure(std::string("open fd ") + std::to_string(fd) + " failed");

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

FileStream::FileStream(const int &fd, long offset, long length) : m_offset(offset) {
    m_canWrite = false;
    m_canRead = true;
    m_canSeek = true;
    m_canGetPosition = true;
    m_canGetLength = true;
    m_length = length;
    file = fdopen(fd, "r");
    m_is_raw_file = true;
}


FileStream::~FileStream() {
    if (!m_closed) {
        close();
    }
}

long FileStream::write(void *buffer, long offset, size_t count) {
    if (m_closed)
        throw std::ios_base::failure("The write operation failed because the file was closed ");

    if (fseek(file, m_position + m_offset, SEEK_SET) != 0)
        throw std::ios_base::failure("Seek failed: " + std::string(strerror(errno)));

    const char *pointer = static_cast<char *>(buffer) + offset;
    size_t writeBytes = fwrite(pointer, 1, count, file);

    if (ferror(file))
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
    if (fseek(file, m_offset + m_position, SEEK_SET) != 0)
        throw std::ios_base::failure("Seek failed: " + std::string(strerror(errno)));

    char *pointer = static_cast<char *>(buffer) + offset;
    size_t actualRead = fread(pointer, 1, readBytes, file);

    if (ferror(file))
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
    if (m_closed)
        return;

    IStream::close();

    if (!m_is_raw_file && file) {
        if (fclose(file) != 0) {
            throw std::ios_base::failure("Close failed: " + std::string(strerror(errno)));
        }
    }
    file = nullptr;
    m_closed = true;
}

void FileStream::Export(napi_env env, napi_value exports) {
//    napi_property_descriptor desc[] = {
//        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
//    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, 0, nullptr, &napi_cons);
    Extends(env, napi_cons);

    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}

napi_value FileStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(2);

    napi_valuetype type;

    NAPI_CALL(env, napi_typeof(env, argv[0], &type))


    int mode = getInt(env, argv[1]);
//    FileStream *stream = nullptr;
    std::shared_ptr<IStream> stream;
    try {
        if (type == napi_string) {
            std::string path = getString(env, argv[0]);
            stream = std::make_shared<FileStream>(path, FILE_MODE(mode), DEFAULT_BUFFER_SIZE);
        } else if (type == napi_number) {
            int fd = getInt(env, argv[0]);
            stream = std::make_shared<FileStream>(fd, FILE_MODE(mode), DEFAULT_BUFFER_SIZE);
        } else {
            napi_value js_fd = nullptr;
            napi_value js_offset = nullptr;
            napi_value js_length = nullptr;
            NAPI_CALL(env, napi_get_named_property(env, argv[0], "fd", &js_fd))
            NAPI_CALL(env, napi_get_named_property(env, argv[0], "offset", &js_offset))
            NAPI_CALL(env, napi_get_named_property(env, argv[0], "length", &js_length))
            int fd = 0;
            long offset = 0;
            long length = 0;
            NAPI_CALL(env, napi_get_value_int32(env, js_fd, &fd))
            NAPI_CALL(env, napi_get_value_int64(env, js_offset, &offset))
            NAPI_CALL(env, napi_get_value_int64(env, js_length, &length))
            stream = std::make_shared<FileStream>(fd, offset, length);
        }
    } catch (const std::ios_base::failure &e) {
        napi_throw_error(env, "JSFileStream", e.what());
        return nullptr;
    }

//    napi_wrap(env, _this, stream, JSDispose, nullptr, nullptr);
//    return _this;
    return JSBind(env, _this, stream);
}

// void FileStream::JSDispose(napi_env env, void *data, void *hint) {
//     FileStream *stream = static_cast<FileStream *>(data);
//     stream->close();
//
//     delete stream;
// }

void FileStream::setLength(long length) {
    if (-1 == ftruncate(fileno(file), length)) {
        throw std::ios::failure("set length failed");
    }

    m_position = std::min(length, m_position);
    m_length = length;
}
