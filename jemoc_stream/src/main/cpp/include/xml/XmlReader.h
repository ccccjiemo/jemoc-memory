//
// Created on 2025/1/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_XMLREADER_H
#define JEMOC_STREAM_TEST_XMLREADER_H

#include <sys/types.h>
#include <string>

enum XMLNodeType {};

class XmlReader {
    virtual uint getAttributeCount() const;
    virtual std::string getBaseURI() const;
    virtual bool getCanReadBinaryContent() const;
    virtual bool getCanReadValueChunk() const;
    virtual bool getCanResolveEntity() const;
    virtual uint getDepth() const;
    virtual bool getEOF() const;
    virtual bool getHasAttributes() const;
    virtual bool getHasValue() const;
    virtual std::string getLocalName() const;
    virtual std::string getName() const;
    virtual std::string getNamespaceURI() const;
    virtual XMLNodeType getNodeType() const;
    virtual std::string getPrefix() const;
    virtual std::string getValue() const;
    virtual std::string getValueType() const;
    virtual std::string getXmlLang() const;
    virtual std::string getXmlSpace() const;
};

#endif // JEMOC_STREAM_TEST_XMLREADER_H
