//
// Created on 2025/1/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_FILESTREAM_H
#define JEMOC_STREAM_TEST_FILESTREAM_H
#include "IStream.h"
#include <fstream>


enum FILE_MODE {
    FILE_MODE_READ = 0x00,
    FILE_MODE_WRITE = 0x01,
    FILE_MODE_APPEND = 0x02,
    FILE_MODE_TRUNC = 0x04,
    FILE_MODE_CREATE = 0x08
};


class FileStream : public IStream {
public:
    FileStream(const std::string &path, FILE_MODE mode, long bufferSize);
    FileStream(const int &fd, FILE_MODE mode, long bufferSize);
    FileStream(const int &fd, long offset, long length);
    ~FileStream();
    long write(void *buffer, long offset, size_t count) override;
    long read(void *buffer, long offset, size_t count) override;
    void flush() override;
    void close() override;
    void setLength(long length) override;

public:
    static std::string ClassName;
    static napi_ref cons;
    static void Export(napi_env env, napi_value exports);
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
//    static void JSDispose(napi_env env, void *data, void *hint);

private:
    FILE_MODE m_mode;
    long m_bufferSize;
    FILE *file = nullptr;
    long m_offset = 0;
    bool m_is_raw_file = false;
};


#endif // JEMOC_STREAM_TEST_FILESTREAM_H
