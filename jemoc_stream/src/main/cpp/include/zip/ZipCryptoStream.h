//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
#define JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
#include "IStream.h"
#include "zlib-ng.h"
#include "crypt.h"
#include "common.h"
#define ZipCryptoStream_DefaultBufferSize 8192  

class ZipArchiveEntry;

enum CryptoMode { CryptoMode_Decode, CryptoMode_Encode };

class ZipCryptoStream : public IStream {
public:
    ZipCryptoStream(std::shared_ptr<IStream> stream, CryptoMode mode, const std::string &password, bool leaveOpen,
                    unsigned long crc = 0, size_t bufferSize = ZipCryptoStream_DefaultBufferSize);
    ZipCryptoStream(std::shared_ptr<IStream> stream, CryptoMode mode, ZipArchiveEntry *entry, bool leaveOpen,
                    size_t bufferSize = ZipCryptoStream_DefaultBufferSize);
    ~ZipCryptoStream();

    long read(void *buffer, long offset, size_t count) override;
    long write(void *buffer, long offset, size_t count) override;
    void close() override;
    long getPosition() const override;
    long getLength() const override;


    static std::string ClassName;
    static napi_ref cons;
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static void Export(napi_env env, napi_value exports);

private:
    void writeCrypthead();
    void ensureNotClose();
    void checkStream();
    void ensureBuffer();
    void decodeStream(void *buffer, long offset, size_t count);
    void encodeStream(void *buffer, long offset, size_t count);

private:
    std::shared_ptr<IStream> m_stream;
    CryptoMode m_mode;
    std::string m_password;
    bool m_leaveOpen;
    unsigned long m_crc;
    unsigned long pkeys[3];
    size_t m_total_read = 0;
    uint8_t *m_buffer = nullptr;
    size_t m_buffer_Size;

    ZipArchiveEntry *m_entry = nullptr;
    bool m_everWrite = false;
    unsigned char verify1 = 0;
    unsigned char verify2 = 0;
    bool m_everRead = false;
};

#endif // JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
