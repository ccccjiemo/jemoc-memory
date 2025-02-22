//
// Created on 2025/1/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_CHECKSUMANDSIZEWRITESTREAM_H
#define JEMOC_STREAM_TEST_CHECKSUMANDSIZEWRITESTREAM_H
#include "IStream.h"
#include <sys/types.h>


class CheckSumAndSizeWriteStream : public IStream {
public:
    CheckSumAndSizeWriteStream(std::shared_ptr<IStream> stream, std::shared_ptr<IStream> baseStream ,bool leaveOpen, std::function<void(long, long, uint)> onClose);
    long write(void *buffer, long offset, size_t count) override;
    void close() override;
    void flush() override;

private:
    std::shared_ptr<IStream> m_stream = nullptr;
    std::shared_ptr<IStream> m_baseStream = nullptr;
    bool m_leaveOpen;
    uint m_checksum = 0;
    bool m_everWritten = false;
    long m_initialPosition = 0;
    std::function<void(long, long, uint)> m_onClose;
};

#endif // JEMOC_STREAM_TEST_CHECKSUMANDSIZEWRITESTREAM_H
