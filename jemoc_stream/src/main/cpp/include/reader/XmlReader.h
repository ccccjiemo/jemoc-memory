//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_XMLREADER_H
#define JEMOC_STREAM_TEST_XMLREADER_H
#include "TextReader.h"
#include "common.h"
#include "reader/StreamReader.h"
#include "reader/StringReader.h"
#include <map>
#include <stack>
#include <string>
#include <cctype>
#include <vector>


enum class XmlNodeType { 
    None, 
    Element, 
    Attribute, 
    Text, 
    CDATA, 
    Comment, 
    Document, 
    EndElement, 
    XmlDeclaration,
    DocumentType,
    ProcessingInstruction,
    Whitespace 
};

class XmlReader {
private:
    std::shared_ptr<TextReader> reader;
    std::string name;
    std::string value;
    XmlNodeType nodeType;
    std::map<std::string, std::string> attributes;
    std::stack<std::string> elementStack;
    bool isEmptyElement;
    std::string version;
    std::string encoding;
    bool isStandalone;
    std::vector<std::string> entityStack;

    // 状态跟踪
    bool inXmlDeclaration = false;
    bool inDocumentType = false;
    bool inProcessingInstruction = false;

public:
    explicit XmlReader(std::shared_ptr<TextReader> reader)
        : reader(reader), nodeType(XmlNodeType::None), isEmptyElement(false),
          isStandalone(false) {
        if (!reader) {
            throw std::invalid_argument("reader cannot be null");
        }
    }

    explicit XmlReader(const std::string &xmlStr) : XmlReader(std::make_shared<StringReader>(xmlStr)) {}

    explicit XmlReader(std::shared_ptr<IStream> stream, bool leaveOpen)
        : XmlReader(std::make_shared<StreamReader>(stream, true, "UTF-8", leaveOpen)) {}

    bool Read() {
        SkipWhitespace();

        int ch = reader->Peek();
        if (ch == -1)
            return false;

        name.clear();
        value.clear();
        attributes.clear();

        if (ch == '<') {
            HandleTagStart();
        } else {
            HandleTextContent();
        }

        return true;
    }

    XmlNodeType GetNodeType() const { return nodeType; }
    const std::string &GetName() const { return name; }
    const std::string &GetValue() const { return value; }
    std::string GetAttribute(const std::string &attrName) const {
        auto it = attributes.find(attrName);
        return it != attributes.end() ? it->second : std::string();
    }
    std::map<std::string, std::string> GetAttributes() const { return attributes; }
    bool IsEmptyElement() const { return isEmptyElement; }
    std::string GetVersion() const { return version; }
    std::string GetEncoding() const { return encoding; }
    bool IsStandalone() const { return isStandalone; }

private:
    // 新增的完整方法实现
    void ReadName() {
        int ch;
        while ((ch = reader->Peek()) != -1) {
            if (std::isalnum(ch) || ch == '_' || ch == '-' || ch == ':' || ch == '.') {
                name += static_cast<char>(reader->Read());
            } else {
                break;
            }
        }
    }

    void ReadProcessingInstruction() {
        reader->Read(); // 跳过'?'
        ReadName();
        
        // 读取处理指令内容直到?>
        int ch;
        while ((ch = reader->Read()) != -1) {
            if (ch == '?' && reader->Peek() == '>') {
                reader->Read(); // 跳过'>'
                break;
            }
            value += static_cast<char>(ch);
        }
    }

    void ReadDocumentType() {
        reader->Read(); // 跳过'D'
        std::string doctype;
        int depth = 1;
        int ch;
        
        while ((ch = reader->Read()) != -1) {
            if (ch == '<') {
                depth++;
            } else if (ch == '>') {
                if (--depth == 0) break;
            }
            doctype += static_cast<char>(ch);
        }
        
        // 解析DOCTYPE内容
        size_t pos = doctype.find(' ');
        if (pos != std::string::npos) {
            name = doctype.substr(0, pos);
            value = doctype.substr(pos + 1);
        } else {
            name = doctype;
        }
    }

    void ReadAttributes() {
        while (true) {
            SkipWhitespace();
            int ch = reader->Peek();

            if (ch == '>' || ch == '/' || ch == '?' || ch == -1)
                break;
            name.clear(); // 清空name为后续使用

            // 正确获取属性名
            ReadName(); // 读取到name成员变量
            std::string attrName = name;
            
            SkipWhitespace();
            if (reader->Read() != '=')
                throw std::runtime_error("Invalid attribute syntax");

            SkipWhitespace();
            char quote = static_cast<char>(reader->Read());
            if (quote != '"' && quote != '\'')
                throw std::runtime_error("Invalid attribute value quoting");

            std::string attrValue;
            while ((ch = reader->Peek()) != -1 && ch != quote) {
                if (ch == '&') {
                    attrValue += ReadEntity();
                } else {
                    attrValue += static_cast<char>(reader->Read());
                }
            }
            reader->Read(); // 跳过结束引号

            attributes[attrName] = attrValue;
        }
    }

    void ReadText() {
        int ch;
        while ((ch = reader->Peek()) != -1 && ch != '<') {
            if (ch == '&') {
                value += ReadEntity();
            } else {
                value += static_cast<char>(reader->Read());
            }
        }
    }

    // 实体解析方法
    std::string ReadEntity() {
        reader->Read(); // 跳过'&'
        std::string entity;
        int ch;
        
        while ((ch = reader->Read()) != -1) {
            if (ch == ';') break;
            entity += static_cast<char>(ch);
        }
        
        if (entity == "lt") return "<";
        if (entity == "gt") return ">";
        if (entity == "amp") return "&";
        if (entity == "apos") return "'";
        if (entity == "quot") return "\"";
        if (entity[0] == '#') {
            int code = (entity[1] == 'x') 
                ? std::stoi(entity.substr(2), nullptr, 16)
                : std::stoi(entity.substr(1));
            return std::string(1, static_cast<char>(code));
        }
        return "&" + entity + ";"; // 未知实体保持原样
    }

    // 原有状态处理方法保持不变
    void HandleTagStart() {
        reader->Read(); // Skip '<'
        int ch = reader->Peek();

        if (ch == '/') {
            HandleEndElement();
        } else if (ch == '?') {
            HandleProcessingInstruction();
        } else if (ch == '!') {
            HandleSpecialTag();
        } else {
            HandleStartElement();
        }
    }

    void HandleEndElement() {
        reader->Read(); // Skip '/'
        ReadName();
        nodeType = XmlNodeType::EndElement;
        SkipToEndOfElement();
        if (!elementStack.empty() && elementStack.top() == name) {
            elementStack.pop();
        }
    }

    void HandleProcessingInstruction() {
        reader->Read(); // Skip '?'
        if (reader->Peek() == 'x' || reader->Peek() == 'X') {
            ReadXmlDeclaration();
            nodeType = XmlNodeType::XmlDeclaration;
        } else {
            ReadProcessingInstruction();
            nodeType = XmlNodeType::ProcessingInstruction;
        }
    }

    void HandleSpecialTag() {
        reader->Read(); // Skip '!'
        int nextCh = reader->Peek();
        
        if (nextCh == 'D') {
            ReadDocumentType();
            nodeType = XmlNodeType::DocumentType;
        } else if (nextCh == '-') {
            ReadComment();
            nodeType = XmlNodeType::Comment;
        } else if (nextCh == '[') {
            ReadCDATA();
            nodeType = XmlNodeType::CDATA;
        }
    }

    void HandleStartElement() {
        ReadName();
        std::string localName = name; // 保存元素名称
        nodeType = XmlNodeType::Element;
        elementStack.push(localName);
        ReadAttributes();
        name = localName; // 恢复元素名称

        int ch = reader->Peek();
        if (ch == '/') {
            isEmptyElement = true;
            reader->Read(); // Skip '/'
            reader->Read(); // Skip '>'
            elementStack.pop();
        } else {
            isEmptyElement = false;
            reader->Read(); // Skip '>'
        }
    }

    void HandleTextContent() {
        ReadText();
        nodeType = std::all_of(value.begin(), value.end(), [](char c) { 
            return std::isspace(c); 
        }) ? XmlNodeType::Whitespace : XmlNodeType::Text;
    }

    // 其他辅助方法保持不变
    void SkipWhitespace() {
        int ch;
        while ((ch = reader->Peek()) != -1 && std::isspace(ch)) {
            reader->Read();
        }
    }

    void SkipToEndOfElement() {
        int ch;
        while ((ch = reader->Peek()) != -1 && ch != '>') {
            reader->Read();
        }
        if (ch == '>') {
            reader->Read();
        }
    }

    void ReadXmlDeclaration() {
        ReadName(); // Should read "xml"
        std::string declarationName = name; // 保存声明名称
        ReadAttributes();
        name = declarationName; // 恢复声明名称
        
        version = GetAttribute("version");
        encoding = GetAttribute("encoding");
        std::string standalone = GetAttribute("standalone");
        isStandalone = (standalone == "yes" || standalone == "true");
        
        if (version.empty()) {
            throw std::runtime_error("XML declaration must have version attribute");
        }
        
        // Skip closing ?> 
        if (reader->Read() != '?' || reader->Read() != '>') {
            throw std::runtime_error("Invalid XML declaration closing");
        }
    }

    void ReadCDATA() {
        // Verify CDATA start
        const char* expected = "CDATA[";
        for (int i = 0; i < 6; ++i) {
            if (reader->Read() != expected[i]) {
                throw std::runtime_error("Invalid CDATA section");
            }
        }

        int consecutiveBrackets = 0;
        int ch;
        while ((ch = reader->Read()) != -1) {
            if (ch == ']') {
                consecutiveBrackets++;
                if (consecutiveBrackets >= 2) {
                    if (reader->Peek() == '>') {
                        reader->Read(); // Skip '>'
                        return;
                    }
                    consecutiveBrackets = 0;
                    value += "]]";
                }
            } else {
                if (consecutiveBrackets > 0) {
                    value.append(consecutiveBrackets, ']');
                    consecutiveBrackets = 0;
                }
                value += static_cast<char>(ch);
            }
        }
        throw std::runtime_error("Unclosed CDATA section");
    }

    void ReadComment() {
        // Verify comment start
        if (reader->Read() != '-' || reader->Read() != '-') {
            throw std::runtime_error("Invalid comment start");
        }

        int dashCount = 0;
        int ch;
        while ((ch = reader->Read()) != -1) {
            if (ch == '-') {
                dashCount++;
                if (dashCount >= 2) {
                    if (reader->Read() == '>') {
                        return;
                    }
                    dashCount = 0;
                    value += "--";
                }
            } else {
                if (dashCount > 0) {
                    value.append(dashCount, '-');
                    dashCount = 0;
                }
                value += static_cast<char>(ch);
            }
        }
        throw std::runtime_error("Unclosed comment");
    }
};

#endif // JEMOC_STREAM_TEST_XMLREADER_H
