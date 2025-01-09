//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
#define JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
#include "IStream.h"

enum CryptoMode { CryptoMode_Decode, CryptoMode_Encode };

class ZipCryptoStream : public IStream {
public:
    ZipCryptoStream(IStream *stream, CryptoMode mode, const std::string &password, bool leaveOpen, unsigned long crc);
    ~ZipCryptoStream();

    long read(void *buffer, long offset, size_t count) override;
    long write(void *buffer, long offset, size_t count) override;
    void close() override;
    long getPosition() const override;
    long getLength() const override;

private:
    void writeCrypthead();
    void ensureNotClose();
    void checkStream();

private:
    IStream *m_stream = nullptr;
    CryptoMode m_mode;
    const std::string m_password;
    bool m_leaveOpen;
    unsigned long m_crc;
    bool m_hasCrc = false;
    unsigned long pkeys[3];
    size_t m_total_read = 0;
    uint8_t *m_buffer = nullptr;
    size_t m_buffer_Size;
};

#endif // JEMOC_STREAM_TEST_ZIPCRYPTOSTREAM_H
