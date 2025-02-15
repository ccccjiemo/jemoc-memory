//
// Created on 2025/2/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/MemfdStream.h"
#include <sys/sendfile.h>
#include "IStream.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <sys/stat.h>

std::string MemfdStream::ClassName = "MemfdStream";
napi_ref MemfdStream::cons = nullptr;

MemfdStream::MemfdStream() : m_fd(-1) {
    // 创建内存中的匿名文件，设置 MFD_CLOEXEC 参数
    // 生成唯一共享内存名称
    static int counter = 0;
    char shm_name[256];
    snprintf(shm_name, sizeof(shm_name), "/jemoc_memfd_%d_%ld_%d", getpid(), time(nullptr), counter++);

    // 使用shm_open创建共享内存对象
    m_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (m_fd < 0) {
        throw std::runtime_error(std::string("shm_open failed: ") + std::strerror(errno));
    }
    m_canRead = true;
    m_canWrite = true;
    m_canSeek = true;
    m_canGetLength = true;
    m_canGetPosition = true;
    m_canSetLength = true;
    m_position = 0;
    m_length = 0;
    m_closed = false;
}

MemfdStream::MemfdStream(const void *initialBuffer, size_t bufferSize) : m_fd(-1) {
    // 创建内存中的匿名文件
    // 生成唯一共享内存名称
    static int counter = 0;
    char shm_name[256];
    snprintf(shm_name, sizeof(shm_name), "/jemoc_memfd_%d_%ld_%d", getpid(), time(nullptr), counter++);

    // 使用shm_open创建共享内存对象
    m_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (m_fd < 0) {
        throw std::runtime_error(std::string("shm_open failed: ") + std::strerror(errno));
    }
    m_canRead = true;
    m_canWrite = true;
    m_canSeek = true;
    m_canGetLength = true;
    m_canGetPosition = true;
    m_canSetLength = true;
    m_position = 0;
    m_length = 0;
    m_closed = false;

    // 如果提供了初始缓冲区，则写入数据
    if (initialBuffer && bufferSize > 0) {
        ssize_t bytesWritten = ::write(m_fd, initialBuffer, bufferSize);
        if (bytesWritten < 0) {
            throw std::runtime_error(std::string("write initialBuffer failed: ") + std::strerror(errno));
        }
        // 更新流长度，并将当前位置设置到流末端
        m_length = bytesWritten;
        m_position = m_length;
    }
}

MemfdStream::~MemfdStream() {
    if (m_closed)
        return;
    IStream::close();
    close();
}

long MemfdStream::read(void *buffer, long offset, size_t count) {
    if (m_closed) {
        throw std::runtime_error("read on closed stream");
    }
    // 定位到当前流位置
    if (lseek(m_fd, m_position, SEEK_SET) == -1) {
        throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));
    }
    char *buf = static_cast<char *>(buffer);
    ssize_t bytesRead = ::read(m_fd, buf + offset, count);
    if (bytesRead < 0) {
        throw std::runtime_error(std::string("read failed: ") + std::strerror(errno));
    }
    m_position += bytesRead;
    return bytesRead;
}

long MemfdStream::write(void *buffer, long offset, size_t count) {
    if (m_closed) {
        throw std::runtime_error("write on closed stream");
    }
    // 定位到当前流位置
    if (lseek(m_fd, m_position, SEEK_SET) == -1) {
        throw std::runtime_error(std::string("lseek failed: ") + std::strerror(errno));
    }
    char *buf = static_cast<char *>(buffer);
    ssize_t bytesWritten = ::write(m_fd, buf + offset, count);
    if (bytesWritten < 0) {
        throw std::runtime_error(std::string("write failed: ") + std::strerror(errno));
    }
    m_position += bytesWritten;
    if (m_position > m_length) {
        m_length = m_position; // 更新流长度
    }
    return bytesWritten;
}

long MemfdStream::seek(long offset, SeekOrigin origin) {
    if (m_closed) {
        throw std::runtime_error("seek on closed stream");
    }
    int whence;
    switch (origin) {
    case Begin:
        whence = SEEK_SET;
        break;
    case Current:
        whence = SEEK_CUR;
        break;
    case End:
        whence = SEEK_END;
        break;
    default:
        throw std::runtime_error("invalid seek origin");
    }
    off_t pos = lseek(m_fd, offset, whence);
    if (pos == -1) {
        throw std::runtime_error(std::string("seek failed: ") + std::strerror(errno));
    }
    m_position = pos;
    return pos;
}

void MemfdStream::flush() {
    if (m_closed) {
        throw std::runtime_error("flush on closed stream");
    }
    if (fsync(m_fd) == -1) {
        throw std::runtime_error(std::string("flush failed: ") + std::strerror(errno));
    }
}

void MemfdStream::close() {
    if (!m_closed) {
        IStream::close();
        ::close(m_fd);
    }
}

void MemfdStream::setLength(long length) {
    if (m_closed) {
        throw std::runtime_error("setLength on closed stream");
    }
    // 使用 ftruncate 截断流
    if (ftruncate(m_fd, length) == -1) {
        throw std::runtime_error(std::string("setLength failed: ") + std::strerror(errno));
    }
    m_length = length;
    // 如果当前位置超出新长度，则调整到末尾
    if (m_position > m_length) {
        m_position = m_length;
    }
}

void MemfdStream::sendFile(const int &fd) {
    // 通过 fstat 获取源文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        throw std::runtime_error(std::string("fstat failed: ") + std::strerror(errno));
    }
    off_t file_size = st.st_size;
    off_t offset = 0;
    ssize_t totalSent = 0;

    // 使用 sendfile 将数据拷贝到目标 fd
    while (offset < file_size) {
        ssize_t sent = sendfile(fd, m_fd, &offset, file_size - offset);
        if (sent <= 0) {
            throw std::runtime_error(std::string("sendfile failed: ") + std::strerror(errno));
        }
        totalSent += sent;
    }
}


napi_value MemfdStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(1)

    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))

    MemfdStream *stream = nullptr;

    try {
        if (type == napi_undefined) {
            stream = new MemfdStream();
        } else {
            void *data = nullptr;
            size_t length = 0;
            getBuffer(env, argv[0], &data, &length);
            stream = new MemfdStream(data, length);
        }
    } catch (const std::exception &e) {
        napi_throw_error(env, tagName, e.what());
        return nullptr;
    }
    NAPI_CALL(env, napi_wrap(env, _this, stream, JSDisposed, nullptr, nullptr))
    return _this;
}

void MemfdStream::JSDisposed(napi_env env, void *data, void *hint) {
    try {
        MemfdStream *stream = static_cast<MemfdStream *>(data);
        stream->close();
        delete stream;
    } catch (const std::exception &e) {
    }
}

napi_value MemfdStream::JSToArrayBuffer(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    try {
        napi_value result = static_cast<MemfdStream *>(stream)->readAllFromFd(env);
        return result;
    } catch (const std::exception &e) {
        NAPI_CALL(env, napi_throw_error(env, tagName, e.what()))
        return nullptr;
    }
}

napi_value MemfdStream::readAllFromFd(napi_env env) {
    // 获取文件大小
    void *data = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, m_length, &data, &result))

    // 使用 pread 从偏移量 0 读取数据，prea不会改变文件指针位置
    ssize_t bytesRead = pread(m_fd, data, m_length, 0);
    if (bytesRead < 0) {
        throw std::runtime_error(std::string("pread failed: ") + std::strerror(errno));
    }
    return result;
}

napi_value MemfdStream::JSGetFd(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    napi_value result = nullptr;
    int fd = static_cast<MemfdStream *>(stream)->getFd();

    NAPI_CALL(env, napi_create_int32(env, fd, &result));
    return result;
}

napi_value MemfdStream::JSSendFile(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    int fd = 0;

    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[0], &fd))
    } else {
        napi_throw_type_error(env, tagName, "get fd failed");
        return nullptr;
    }

    try {
        static_cast<MemfdStream *>(stream)->sendFile(fd);
    } catch (const std::exception &e) {
        napi_throw_error(env, tagName, e.what());
    }
    return nullptr;
}


void MemfdStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
        DEFINE_NAPI_FUNCTION("fd", nullptr, JSGetFd, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("toArrayBuffer", JSToArrayBuffer, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("sendFile", JSSendFile, nullptr, nullptr, nullptr)
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);

    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}

int MemfdStream::getFd() const { return m_fd; }
