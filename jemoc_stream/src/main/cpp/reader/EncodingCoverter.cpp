//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "reader/EncodingConverter.h"

IconvConverter::IconvConverter(const std::string& fromEncoding) 
    : cd(iconv_open("UTF-8", fromEncoding.c_str())) {
    if (cd.get() == (iconv_t)-1) {
        throw std::runtime_error("Failed to initialize iconv converter");
    }
}

bool IconvConverter::Convert(const char* from, size_t fromSize,
                           char* to, size_t toSize,
                           size_t& convertedChars) {
    char* inbuf = const_cast<char*>(from);
    size_t inbytesleft = fromSize;
    size_t outbytesleft = toSize ;
    char* outbuf = reinterpret_cast<char*>(to);
    
    iconv(cd.get(), nullptr, nullptr, nullptr, nullptr);
    
    size_t result = iconv(cd.get(), &inbuf, &inbytesleft,
                        &outbuf, &outbytesleft);
    
    if (result == (size_t)-1 && errno != E2BIG) {
        return false;
    }

    convertedChars = (toSize * sizeof(char32_t) - outbytesleft) / sizeof(char32_t);
    return true;
}

// EncodingConverterFactory implementation
std::unique_ptr<EncodingConverter> EncodingConverterFactory::Create(const std::string& encoding) {
    try {
        return std::make_unique<IconvConverter>(encoding);
    } catch (const std::exception&) {
        return nullptr;
    }
}

std::vector<std::string> EncodingConverterFactory::GetSupportedEncodings() {
    return {
        "UTF-8",
        "UTF-16BE", "UTF-16LE", "UTF-16",
        "UTF-32BE", "UTF-32LE", "UTF-32",
        "ASCII",
        "GB18030", "GBK", "GB2312",
        "BIG5",
        "EUC-JP", "SHIFT-JIS", "ISO-2022-JP",
        "EUC-KR",
        "ISO-8859-1", "ISO-8859-2", "ISO-8859-3", "ISO-8859-4",
        "ISO-8859-5", "ISO-8859-6", "ISO-8859-7", "ISO-8859-8",
        "ISO-8859-9", "ISO-8859-10", "ISO-8859-11", "ISO-8859-13",
        "ISO-8859-14", "ISO-8859-15", "ISO-8859-16",
        "WINDOWS-1250", "WINDOWS-1251", "WINDOWS-1252", "WINDOWS-1253",
        "WINDOWS-1254", "WINDOWS-1255", "WINDOWS-1256", "WINDOWS-1257",
        "WINDOWS-1258",
        "KOI8-R", "KOI8-U",
        "CP437", "CP850", "CP866", "CP1047"
    };
} 