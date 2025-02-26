//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_STREAMREADERBINDING_H
#define JEMOC_STREAM_TEST_STREAMREADERBINDING_H

#include "binding/TextReaderBinding.h"
#include "binding/binding.h"
#include "reader/StreamReader.h"
#include <algorithm> // std::transform
#include <cctype>    // std::toupper

class StreamReaderBinding;

struct StreamReaderOptions {
    bool detectEncodingFromByteOrderMarks;
    std::string encoding;
    bool leaveOpen;
};

template <>
template <>
StreamReaderOptions JSBinding<StreamReader, StreamReaderBinding>::ParseArg<StreamReaderOptions>(napi_env env,
                                                                                                napi_value arg) {
    StreamReaderOptions options{.detectEncodingFromByteOrderMarks = true, .encoding = "UTF-8", .leaveOpen = false};

    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, arg, &type))
    if (type == napi_object) {
        napi_value v;
        NAPI_CALL(env, napi_get_named_property(env, arg, "detectEncodingFromByteOrderMarks", &v))
        As<bool>(env, v, &options.detectEncodingFromByteOrderMarks);
        v = nullptr;
        NAPI_CALL(env, napi_get_named_property(env, arg, "encoding", &v))
        if (As<std::string>(env, v, &options.encoding)) {
            std::transform(options.encoding.begin(), options.encoding.end(), options.encoding.begin(), ::toupper);
        }
        v = nullptr;
        NAPI_CALL(env, napi_get_named_property(env, arg, "leaveOpen", &v))
        As<bool>(env, v, &options.leaveOpen);
    }
    return options;
}

class StreamReaderBinding : public JSBinding<StreamReader, StreamReaderBinding> {
// public:
//     struct Traits : public DefaultJSBindingTraits<StreamReaderBinding> {
//         static std::string GetClassName() { return "StreamReader"; }
//         static InstanceType ConstructInstance(napi_env env, size_t argc, napi_value *argv) {
//             StreamReaderOptions options = ParseArg<StreamReaderOptions>(env, argc == 1 ? nullptr : argv[1]);
//             std::string path;
//
//             if (As<std::string>(env, argv[0], &path)) {
//                 return StoragePolicyType::Create(path, options.detectEncodingFromByteOrderMarks,
    //                 options.encoding,
//                                                  options.leaveOpen);
//             } else {
//                 std::shared_ptr<IStream> stream = IStream::GetStream(env, argv[0]);
//                 if (!stream)
//                     throw std::invalid_argument("stream is null");
//                 return StoragePolicyType::Create(stream, options.detectEncodingFromByteOrderMarks,
    //                 options.encoding,
//                                                  options.leaveOpen);
//             }
//         }
//         static std::string GetParentName() { return "TextReader"; }
//         static std::vector<napi_property_descriptor> GetMethods(napi_env env) {
//             return {
//                 DEFINE_GETTER("encoding", GetEncoding),
//                 DEFINE_GETTER("endOfStream", GetEndOfStream),
//             };
//         }
//     };
    DECLARE_BINDING(StreamReader, StreamReaderBinding)
    INHERITS(TextReaderBinding::TraitsType::GetClassName())
    METHODS(DEFINE_GETTER("encoding", GetEncoding), DEFINE_GETTER("endOfStream", GetEndOfStream))
    CONSTRUCTOR(Constructor)
    END_BINDING

protected:
    static InstanceType Constructor(napi_env env, size_t argc, napi_value *argv) {
        StreamReaderOptions options = ParseArg<StreamReaderOptions>(env, argc == 1 ? nullptr : argv[1]);
        std::string path;

        if (As<std::string>(env, argv[0], &path)) {
            return StoragePolicyType::Create(path, options.detectEncodingFromByteOrderMarks, options.encoding,
                                             options.leaveOpen);
        } else {
            std::shared_ptr<IStream> stream = IStream::GetStream(env, argv[0]);
            if (!stream)
                throw std::invalid_argument("stream is null");
            return StoragePolicyType::Create(stream, options.detectEncodingFromByteOrderMarks, options.encoding,
                                             options.leaveOpen);
        }
    }
    static std::string GetEncoding(StreamReader *reader) { return reader->GetEncoding(); }
    static bool GetEndOfStream(StreamReader *reader) { return reader->EndOfStream(); }
};


#endif // JEMOC_STREAM_TEST_STREAMREADERBINDING_H
