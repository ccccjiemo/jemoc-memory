//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_TEXTREADER_H
#define JEMOC_STREAM_TEST_TEXTREADER_H

#include <string>
#include <vector>
class TextReader {
protected:
    TextReader() = default;

public:
    virtual ~TextReader() = default;

    virtual int Read() = 0;
    virtual int Read(std::vector<char> &buffer, int index, int count) {
        if (index < 0 || count < 0) {
            throw std::invalid_argument("index and count must be non-negative");
        }
        if (buffer.size() < static_cast<size_t>(index + count)) {
            throw std::out_of_range("buffer too small");
        }
        int n = 0;
        while (n < count) {
            int ch = Read();
            if (ch == -1)
                break;
            buffer[index + n] = static_cast<char>(ch);
            n++;
        }
        return n;
    }

    virtual std::string ReadLine() {
        std::string line;
        int ch;
        while ((ch = Read()) != -1) {
            if (ch == '\r') {
                if ((ch = Read()) == '\n') {
                    return line;
                }
                line += '\r';
                if (ch == -1)
                    break;
            }
            if (ch == '\n')
                return line;
            line += static_cast<char>(ch);
        }
        return line.empty() && ch == -1 ? std::string() : line;
    }

    virtual std::string ReadToEnd() {
        std::string text;
        std::vector<char> buffer(4096);
        int count;

        while ((count = Read(buffer, 0, buffer.size())) > 0) {
            text.append(buffer.begin(), buffer.begin() + count);
        }
        return text;
    }
    
    virtual int Skip(int count) {
        if(count < 0) {
            throw std::invalid_argument("count must be non-negative");
        }
        int n = 0;
        while(n < count) {
            if(Read() == -1) break;
            n++;
        }
        return n;
    }
    
    virtual int Peek() {
        return -1;
    }
    
    virtual void Close() {
        
    }
};

#endif // JEMOC_STREAM_TEST_TEXTREADER_H
