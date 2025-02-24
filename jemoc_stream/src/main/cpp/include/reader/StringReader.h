//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_STRINGREADER_H
#define JEMOC_STREAM_TEST_STRINGREADER_H
#include "TextReader.h"
class StringReader : public TextReader {
private:
    std::string content;
    size_t position;
    

public:
    // 从已有字符串构造
    explicit StringReader(const std::string& str) 
        : content(str), position(0) {}

    // 从右值字符串构造（避免拷贝）
    explicit StringReader(std::string&& str) noexcept
        : content(std::move(str)), position(0) {}

    int Read() override {
        if (position >= content.size()) {
            return -1;
        }
        return static_cast<unsigned char>(content[position++]);
    }

    int Peek() override {
        if (position >= content.size()) {
            return -1;
        }
        return static_cast<unsigned char>(content[position]);
    }

    // 重置读取位置
    void Reset() {
        position = 0;
    }

    // 获取剩余内容
    std::string GetRemaining() const {
        return content.substr(position);
    }


    bool EndOfStream() const {
        return position >= content.size();
    }
};

#endif //JEMOC_STREAM_TEST_STRINGREADER_H
