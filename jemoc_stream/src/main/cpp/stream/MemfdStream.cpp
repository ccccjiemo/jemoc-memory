//
// Created on 2025/2/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/MemfdStream.h"
#include "IStream.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

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

bool MemfdStream::sendFile(const int &fd, long offset, long length) {
    // 通过 fstat 获取源文件大小
    struct stat st;
    if (fstat(fd, &st) == -1) {
        OH_LOG_ERROR(LOG_APP, "fstat failed: %s", std::strerror(errno));
        return false;
    }

    off_t offset_ = std::max(0l, offset);
    long length_ = std::max(0l, std::min(m_length - offset_, length));

    // 使用 sendfile 将数据拷贝到目标 fd
    ssize_t sent = sendfile(fd, m_fd, &offset_, length_);
    if (sent <= 0) {
        OH_LOG_ERROR(LOG_APP, "sendfile failed, errno: %d", errno);
        return false;
    }
    return true;
}


napi_value MemfdStream::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(1)

    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))

//    MemfdStream *stream = nullptr;
    std::shared_ptr<IStream> stream;
    try {
        if (type == napi_undefined) {
            stream = std::make_shared<MemfdStream>();
        } else {
            void *data = nullptr;
            size_t length = 0;
            getBuffer(env, argv[0], &data, &length);
            stream = std::make_shared<MemfdStream>(data, length);
        }
    } catch (const std::exception &e) {
        napi_throw_error(env, tagName, e.what());
        return nullptr;
    }
//    NAPI_CALL(env, napi_wrap(env, _this, stream, JSDisposed, nullptr, nullptr))
//    return _this;
    return JSBind(env, _this, stream);
}

// void MemfdStream::JSDisposed(napi_env env, void *data, void *hint) {
//     try {
//         MemfdStream *stream = static_cast<MemfdStream *>(data);
//         stream->close();
//         delete stream;
//     } catch (const std::exception &e) {
//     }
// }

napi_value MemfdStream::JSToArrayBuffer(napi_env env, napi_callback_info info) {
    GET_JS_INFO(1)
    try {
        long offset = 0;
        long length = stream->getLength();
        if (argc > 0) {
            getToArrayBufferOptions(env, argv[0], &offset, &length);
        }
        napi_value result = static_cast<MemfdStream *>(stream.get())->readAllFromFd(env, offset, length);
        return result;
    } catch (const std::exception &e) {
        NAPI_CALL(env, napi_throw_error(env, tagName, e.what()))
        return nullptr;
    }
}

napi_value MemfdStream::readAllFromFd(napi_env env, long offset, long length) {
    // 获取文件大小
    void *data = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, length, &data, &result))

    // 使用 pread 从偏移量 0 读取数据，prea不会改变文件指针位置
    ssize_t bytesRead = pread(m_fd, data, length, offset);
    if (bytesRead < 0) {
        throw std::runtime_error(std::string("pread failed: ") + std::strerror(errno));
    }
    return result;
}

napi_value MemfdStream::JSGetFd(napi_env env, napi_callback_info info) {
    GET_JS_INFO(0)
    napi_value result = nullptr;
    int fd = static_cast<MemfdStream *>(stream.get())->getFd();

    NAPI_CALL(env, napi_create_int32(env, fd, &result));
    return result;
}

napi_value MemfdStream::JSSendFile(napi_env env, napi_callback_info info) {
//     GET_JS_INFO(3)
//     napi_valuetype type;
//     int fd = -1;
//     napi_value result;
//     bool autoClose = false;
//     long offset = 0;
//     long length = stream->getLength();
//     int optionsIndex = 0;
//
//
//     NAPI_CALL(env, napi_typeof(env, argv[0], &type))
//     if (type == napi_number) {
//         NAPI_CALL(env, napi_get_value_int32(env, argv[0], &fd))
//         optionsIndex = 1;
//     } else if (type == napi_string) {
//         NAPI_CALL(env, napi_typeof(env, argv[1], &type))
//         if (type != napi_number) {
//             napi_throw_type_error(env, "MemfdStream", "mode must be number");
//             return nullptr;
//         }
//         autoClose = true;
//         int openMode = 0;
//         NAPI_CALL(env, napi_get_value_int32(env, argv[1], &openMode))
//
//         size_t bufsize = 0;
//         NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], nullptr, 0, &bufsize))
//         std::unique_ptr<char[]> buffer(new char[bufsize + 1]{'\0'});
//         NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], buffer.get(), bufsize + 1, &bufsize))
//         fd = open(buffer.get(), openMode, 0644);
//
//         optionsIndex = 2;
//     } else {
//         napi_throw_type_error(env, ClassName.c_str(), "invalid parameter type");
//         return nullptr;
//     }
//
//     if (fd == -1) {
//         OH_LOG_ERROR(LOG_APP, "%s", "fd error, fd = -1, errno:%d", errno);
//         NAPI_CALL(env, napi_get_boolean(env, false, &result))
//         return result;
//     }
//
//
//     NAPI_CALL(env, napi_typeof(env, argv[optionsIndex], &type))
//     if (type == napi_object) {
//         napi_value js_offset;
//         napi_value js_length;
//         napi_value js_auto_close;
//         NAPI_CALL(env, napi_get_named_property(env, argv[1], "offset", &js_offset))
//         NAPI_CALL(env, napi_get_named_property(env, argv[1], "length", &js_length))
//         NAPI_CALL(env, napi_get_named_property(env, argv[1], "autoClose", &js_auto_close))
//         NAPI_CALL(env, napi_typeof(env, js_offset, &type))
//         if (type == napi_number) {
//             NAPI_CALL(env, napi_get_value_int64(env, js_offset, &offset))
//         }
//
//         NAPI_CALL(env, napi_typeof(env, js_length, &type))
//         if (type == napi_number) {
//             NAPI_CALL(env, napi_get_value_int64(env, js_length, &length))
//         }
//         if (optionsIndex == 1) {
//             NAPI_CALL(env, napi_typeof(env, js_auto_close, &type))
//             if (type == napi_boolean) {
//                 NAPI_CALL(env, napi_get_value_bool(env, js_auto_close, &autoClose));
//             }
//         }
//     }
//
//     try {
//         static_cast<MemfdStream *>(stream)->sendFile(fd, offset, length);
//         NAPI_CALL(env, napi_get_boolean(env, true, &result))
//     } catch (const std::exception &e) {
//         OH_LOG_ERROR(LOG_APP, "%s", e.what());
//         NAPI_CALL(env, napi_get_boolean(env, false, &result))
//     }
//
//     if (autoClose) {
//         ::close(fd);
//     }
//
//     return result;

    int fd = -1;
    bool autoClose = false;
    long offset = 0;
    long length = 0;
    MemfdStream *stream = nullptr;

    initSendFile(env, info, fd, offset, length, autoClose, &stream);

    bool val = stream->sendFile(fd, offset, length);

    if (autoClose) {
        ::close(fd);
    }

    napi_value result = nullptr;

    NAPI_CALL(env, napi_get_boolean(env, val, &result))
    return result;
}

void MemfdStream::initSendFile(napi_env env, napi_callback_info info, int &fd, long &offset, long &length,
                               bool &autoClose, MemfdStream **fdStream) {
    GET_JS_INFO(3)
    napi_valuetype type;
    fd = -1;
    napi_value result;
    autoClose = false;
    offset = 0;
    length = stream->getLength();
    int optionsIndex = 0;

    *fdStream = static_cast<MemfdStream *>(stream.get());


    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[0], &fd))
        optionsIndex = 1;
    } else if (type == napi_string) {
        NAPI_CALL(env, napi_typeof(env, argv[1], &type))
        if (type != napi_number) {
            napi_throw_type_error(env, "MemfdStream", "mode must be number");
        }
        autoClose = true;
        int openMode = 0;
        NAPI_CALL(env, napi_get_value_int32(env, argv[1], &openMode))

        size_t bufsize = 0;
        NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], nullptr, 0, &bufsize))
        std::unique_ptr<char[]> buffer(new char[bufsize + 1]{'\0'});
        NAPI_CALL(env, napi_get_value_string_utf8(env, argv[0], buffer.get(), bufsize + 1, &bufsize))
        fd = open(buffer.get(), openMode, 0644);

        optionsIndex = 2;
    } else {
        napi_throw_type_error(env, ClassName.c_str(), "invalid parameter type");
    }

    NAPI_CALL(env, napi_typeof(env, argv[optionsIndex], &type))
    if (type == napi_object) {
        napi_value js_offset;
        napi_value js_length;
        napi_value js_auto_close;
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "offset", &js_offset))
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "length", &js_length))
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "autoClose", &js_auto_close))
        NAPI_CALL(env, napi_typeof(env, js_offset, &type))
        if (type == napi_number) {
            NAPI_CALL(env, napi_get_value_int64(env, js_offset, &offset))
        }

        NAPI_CALL(env, napi_typeof(env, js_length, &type))
        if (type == napi_number) {
            NAPI_CALL(env, napi_get_value_int64(env, js_length, &length))
        }
        if (optionsIndex == 1) {
            NAPI_CALL(env, napi_typeof(env, js_auto_close, &type))
            if (type == napi_boolean) {
                NAPI_CALL(env, napi_get_value_bool(env, js_auto_close, &autoClose));
            }
        }
    }
}

napi_value MemfdStream::JSSendFileAsync(napi_env env, napi_callback_info info) {
    int fd = -1;
    bool autoClose = false;
    long offset = 0;
    long length = 0;
    MemfdStream *stream = nullptr;

    initSendFile(env, info, fd, offset, length, autoClose, &stream);

    SendFileData *data = new SendFileData{
        .stream = stream, .fd = fd, .offset = offset, .length = length, .result = false, .autoClose = autoClose};
    napi_value resourceName = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "sendFileAsync", NAPI_AUTO_LENGTH, &resourceName))
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &data->deferred, &result))
    napi_create_async_work(
        env, nullptr, resourceName,
        [](napi_env env, void *data) {
            SendFileData *asyncData = static_cast<SendFileData *>(data);
            asyncData->result = asyncData->stream->sendFile(asyncData->fd, asyncData->offset, asyncData->length);
        },
        [](napi_env env, napi_status status, void *data) {
            SendFileData *asyncData = static_cast<SendFileData *>(data);
            napi_value result = nullptr;
            if (asyncData->autoClose) {
                ::close(asyncData->fd);
            }
            NAPI_CALL(env, napi_get_boolean(env, asyncData->result, &result))
            if (status == napi_ok) {
                NAPI_CALL(env, napi_resolve_deferred(env, asyncData->deferred, result))
            } else {
                NAPI_CALL(env, napi_reject_deferred(env, asyncData->deferred, nullptr))
            }
            NAPI_CALL(env, napi_delete_async_work(env, asyncData->work))
        },
        data, &data->work);

    NAPI_CALL(env, napi_queue_async_work(env, data->work))
    return result;
}


void MemfdStream::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
//        DEFINE_NAPI_ISTREAM_PROPERTY((void *)ClassName.c_str()),
        DEFINE_NAPI_FUNCTION("fd", nullptr, JSGetFd, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("toArrayBuffer", JSToArrayBuffer, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("sendFile", JSSendFile, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("sendFileAsync", JSSendFileAsync, nullptr, nullptr, nullptr),
    };
    napi_value napi_cons = nullptr;
    napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr, sizeof(desc) / sizeof(desc[0]),
                      desc, &napi_cons);
    napi_create_reference(env, napi_cons, 1, &cons);
    Extends(env, napi_cons);
    napi_set_named_property(env, exports, ClassName.c_str(), napi_cons);
}

int MemfdStream::getFd() const { return m_fd; }
