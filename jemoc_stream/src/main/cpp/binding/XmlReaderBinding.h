//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_XMLREADERBINDING_H
#define JEMOC_STREAM_TEST_XMLREADERBINDING_H
#include "binding/TextReaderBinding.h"
#include "binding/binding.h"
#include "reader/XmlReader.h"

class XmlReaderBinding : public JSBinding<XmlReader, XmlReaderBinding> {
public:
//     struct Traits : public DefaultJSBindingTraits<XmlReaderBinding> {
//         static std::string GetClassName() { return "XmlReader"; }
//         static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) {
//             std::string xmlStr;
//             if (As<std::string>(env, argv[0], &xmlStr)) {
//                 return StoragePolicyType::Create(xmlStr);
//             }
//             return StoragePolicyType::Create(TextReaderBinding::GetWrapper(env, argv[0])->GetShared());
//         }
//         static std::vector<napi_property_descriptor> GetMethods() {
//             return {
//                 DEFINE_METHOD("read", Read),
//                 DEFINE_GETTER("name", GetName),
//                 DEFINE_GETTER("value", GetValue),
//                 DEFINE_GETTER("nodeType", GetNodeType),
//                 DEFINE_METHOD("getAttribute", GetAttribute),
//                 DEFINE_GETTER("attributes", GetAttributes),
//                 DEFINE_GETTER("isEmptyElement", GetIsEmptyElement),
//                 {"close", nullptr, Close, nullptr, nullptr, nullptr, napi_default, nullptr},
//             };
//         }
//     };

    DECLARE_BINDING(XmlReader, XmlReaderBinding)
    METHODS(DEFINE_METHOD("read", Read), DEFINE_GETTER("name", GetName), DEFINE_GETTER("value", GetValue),
            DEFINE_GETTER("nodeType", GetNodeType), DEFINE_METHOD("getAttribute", GetAttribute),
            DEFINE_GETTER("attributes", GetAttributes), DEFINE_GETTER("isEmptyElement", GetIsEmptyElement),
            DEFINE_CONSTANT("test", env, 1),
            {"close", nullptr, Close, nullptr, nullptr, nullptr, napi_default, nullptr})
    CONSTRUCTOR(Constructor)
    END_BINDING


protected:
    static InstanceType Constructor(napi_env env, size_t argc, napi_value *argv) {
        std::string xmlStr;
        if (As<std::string>(env, argv[0], &xmlStr)) {
            return StoragePolicyType::Create(xmlStr);
        }
        return StoragePolicyType::Create(TextReaderBinding::GetWrapper(env, argv[0])->GetShared());
    }
    static bool Read(XmlReader *reader) { return reader->Read(); }

    static int GetNodeType(XmlReader *reader) { return static_cast<int>(reader->GetNodeType()); }

    static std::string GetName(XmlReader *reader) { return reader->GetName(); }

    static std::string GetValue(XmlReader *reader) { return reader->GetValue(); }

    static napi_value GetAttribute(XmlReader *reader, napi_env env, size_t argc, napi_value *argv) {
        std::string attrName = ParseArg<std::string>(env, argv[0]);
        return ToNapiValue(env, reader->GetAttribute(attrName));
    }

    static bool GetIsEmptyElement(XmlReader *reader) { return reader->IsEmptyElement(); }

    static std::map<std::string, std::string> GetAttributes(XmlReader *reader) { return reader->GetAttributes(); }

    static napi_value Close(napi_env env, napi_callback_info info) {
        napi_value thisArgs;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisArgs, nullptr))
        NapiWrapper *wrapper = GetWrapper(env, thisArgs);
        if (wrapper != nullptr) {
            RemoveWrapper(wrapper->GetNative());
        }
        return nullptr;
    }
};


#endif // JEMOC_STREAM_TEST_XMLREADERBINDING_H
