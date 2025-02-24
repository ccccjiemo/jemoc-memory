#ifndef JEMOC_STREAM_TEST_XMLTOJSONCONVERTER_H
#define JEMOC_STREAM_TEST_XMLTOJSONCONVERTER_H

#include "reader/XmlReader.h"
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include <map>

namespace json {
    class Value {
    public:
        enum Type { Null, Object, Array, String, Boolean, Number };
        
        Type type;
        std::string stringValue;
        double numberValue;
        bool booleanValue;
        std::map<std::string, std::shared_ptr<Value>> objectValue;
        std::vector<std::shared_ptr<Value>> arrayValue;

        Value() : type(Null) {}
        explicit Value(const std::string& v) : type(String), stringValue(v) {}
        explicit Value(double v) : type(Number), numberValue(v) {}
        explicit Value(bool v) : type(Boolean), booleanValue(v) {}
        
        static std::shared_ptr<Value> createObject() {
            auto val = std::make_shared<Value>();
            val->type = Object;
            return val;
        }
        
        static std::shared_ptr<Value> createArray() {
            auto val = std::make_shared<Value>();
            val->type = Array;
            return val;
        }
        
        std::string toString(int indent = 0) const {
            std::string result;
            std::string indentStr(indent, ' ');
            
            switch (type) {
                case Object:
                    result += "{\n";
                    for (auto it = objectValue.begin(); it != objectValue.end();) {
                        result += indentStr + "  \"" + it->first + "\": " + it->second->toString(indent + 2);
                        if (++it != objectValue.end()) result += ",";
                        result += "\n";
                    }
                    result += indentStr + "}";
                    break;
                case Array:
                    result += "[\n";
                    for (auto it = arrayValue.begin(); it != arrayValue.end();) {
                        result += indentStr + "  " + (*it)->toString(indent + 2);
                        if (++it != arrayValue.end()) result += ",";
                        result += "\n";
                    }
                    result += indentStr + "]";
                    break;
                case String:
                    return "\"" + escapeString(stringValue) + "\"";
                case Number:
                    return std::to_string(numberValue);
                case Boolean:
                    return booleanValue ? "true" : "false";
                default:
                    return "null";
            }
            return result;
        }
        
    private:
        static std::string escapeString(const std::string& input) {
            std::string output;
            for (char c : input) {
                switch (c) {
                    case '"':  output += "\\\""; break;
                    case '\\': output += "\\\\"; break;
                    case '\b': output += "\\b";  break;
                    case '\f': output += "\\f";  break;
                    case '\n': output += "\\n";  break;
                    case '\r': output += "\\r";  break;
                    case '\t': output += "\\t";  break;
                    default:   output += c;      break;
                }
            }
            return output;
        }
    };

    class XmlToJsonConverter {
    public:
        explicit XmlToJsonConverter(XmlReader& reader) : xmlReader(reader) {}
        
        std::string convert() {
            std::stack<std::shared_ptr<Value>> nodeStack;
            std::shared_ptr<Value> root;
            std::string currentKey;
            
            while (xmlReader.Read()) {
                switch (xmlReader.GetNodeType()) {
                    case XmlNodeType::Element: {
                        auto newObject = Value::createObject();
                        auto attributes = xmlReader.GetAttributes();
                        
                        // 处理属性
                        if (!attributes.empty()) {
                            auto attrObject = Value::createObject();
                            for (auto& [key, value] : attributes) {
                                attrObject->objectValue[key] = std::make_shared<Value>(value);
                            }
                            newObject->objectValue["@attributes"] = attrObject;
                        }
                        
                        // 处理父子关系
                        if (nodeStack.empty()) {
                            root = newObject;
                            currentKey = xmlReader.GetName();
                        } else {
                            auto parent = nodeStack.top();
                            if (parent->type == Value::Object) {
                                handleObjectInsert(parent, xmlReader.GetName(), newObject);
                            } else if (parent->type == Value::Array) {
                                parent->arrayValue.push_back(newObject);
                            }
                        }
                        
                        nodeStack.push(newObject);
                        if (xmlReader.IsEmptyElement()) {
                            nodeStack.pop();
                        }
                        break;
                    }
                        
                    case XmlNodeType::EndElement: {
                        if (!nodeStack.empty()) {
                            nodeStack.pop();
                        }
                        break;
                    }
                        
                    case XmlNodeType::Text:
                    case XmlNodeType::CDATA: {
                        if (!nodeStack.empty()) {
                            auto current = nodeStack.top();
                            if (current->objectValue.find("#text") != current->objectValue.end()) {
                                // 合并多个文本节点
                                current->objectValue["#text"]->stringValue += xmlReader.GetValue();
                            } else {
                                current->objectValue["#text"] = std::make_shared<Value>(xmlReader.GetValue());
                            }
                        }
                        break;
                    }
                        
                    default:
                        break;
                }
            }
            
            if (root) {
                auto wrapper = Value::createObject();
                wrapper->objectValue[currentKey] = root;
                return wrapper->toString();
            }
            return "{}";
        }
        
    private:
        XmlReader& xmlReader;
        
        void handleObjectInsert(std::shared_ptr<Value> parent, const std::string& key, 
                              std::shared_ptr<Value> value) {
            // 处理多个同名元素转换为数组
            if (parent->objectValue.find(key) == parent->objectValue.end()) {
                parent->objectValue[key] = value;
            } else {
                auto existing = parent->objectValue[key];
                if (existing->type != Value::Array) {
                    auto array = Value::createArray();
                    array->arrayValue.push_back(existing);
                    array->arrayValue.push_back(value);
                    parent->objectValue[key] = array;
                } else {
                    existing->arrayValue.push_back(value);
                }
            }
        }
    };
}

#endif // JEMOC_STREAM_TEST_XMLTOJSONCONVERTER_H
