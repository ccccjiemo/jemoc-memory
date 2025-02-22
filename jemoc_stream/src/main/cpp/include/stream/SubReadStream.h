//
// Created on 2025/1/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_SUBREADSTREAM_H
#define JEMOC_STREAM_TEST_SUBREADSTREAM_H
#include "IStream.h"

class SubReadStream : public IStream {
public:
    SubReadStream(std::shared_ptr<IStream> stream, long startPosition, size_t maxLength, bool leaveOpen);
    ~SubReadStream();

    void close() override;
    bool getCanRead() const override;
    long read(void* buffer, long offset, size_t count) override ;
    

private:
    std::weak_ptr<IStream> m_stream;
    long m_startInStream;
    long m_endInStream;
    bool m_leaveOpen;
};


#endif // JEMOC_STREAM_TEST_SUBREADSTREAM_H
