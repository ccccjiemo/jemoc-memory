//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ENCODINGCONVERTER_H
#define JEMOC_STREAM_TEST_ENCODINGCONVERTER_H
#include "iostream"
#include <iconv.h>

class EncodingConverter {
public:
    virtual ~EncodingConverter() = default;
    virtual bool Convert(const char *from, size_t fromSize, char *to, size_t toSize, size_t &convertedChars) = 0;
};

class IconvConverter : public EncodingConverter {
private:
    struct IconvCloser {
        void operator()(iconv_t cd) {
            if (cd != (iconv_t)-1)
                iconv_close(cd);
        }
    };
    std::unique_ptr<void, IconvCloser> cd;

public:
    explicit IconvConverter(const std::string &fromEncoding);
    bool Convert(const char *from, size_t fromSize, char *to, size_t toSize, size_t &convertedChars) override;
};

class EncodingConverterFactory {
public:
    static std::unique_ptr<EncodingConverter> Create(const std::string &encoding);
    static std::vector<std::string> GetSupportedEncodings();
};
#endif // JEMOC_STREAM_TEST_ENCODINGCONVERTER_H
