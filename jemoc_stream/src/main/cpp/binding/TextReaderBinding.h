//
// Created on 2025/2/23.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_TEXTREADERBINDING_H
#define JEMOC_STREAM_TEST_TEXTREADERBINDING_H
#include "binding/binding.h"
#include "reader/TextReader.h"

class TextReaderBinding : public JSBinding<TextReader, TextReaderBinding> {

    DECLARE_BINDING(TextReader, TextReaderBinding)
    ABSTRACT_CLASS
    METHODS(DEFINE_METHOD("read", Read), DEFINE_METHOD("readLine", ReadLine), DEFINE_METHOD("readToEnd", ReadToEnd),
            DEFINE_METHOD("skip", Skip), DEFINE_METHOD("peek", Peek),
            {"close", nullptr, Close, nullptr, nullptr, nullptr, napi_default, nullptr})
    END_BINDING

protected:
    static napi_value Read(TextReader *reader, napi_env env, size_t argc, napi_value *argv) {
        if (argc == 0) {
            return ToNapiValue<int>(env, reader->Read());
        }
        if (argc == 1) {
            ArrayBufferData data = ParseArg<ArrayBufferData>(env, argv[0]);
            std::vector<char> buffer(data.as<char>(), data.as<char>() + data.length);
            return ToNapiValue<int>(env, reader->Read(buffer, 0, data.length));
        }
        return nullptr;
    }

    static napi_value ReadLine(TextReader *reader, napi_env env) {
        return ToNapiValue<std::string>(env, reader->ReadLine());
    }

    static napi_value ReadToEnd(TextReader *reader, napi_env env) {
        return ToNapiValue<std::string>(env, reader->ReadToEnd());
    }

    static napi_value Skip(TextReader *reader, napi_env env, size_t argc, napi_value *argv) {
        int count = ParseArg<int>(env, argv[0]);
        return ToNapiValue<int>(env, reader->Skip(count));
    }

    static napi_value Peek(TextReader *reader, napi_env env) { return ToNapiValue<int>(env, reader->Peek()); }

    static napi_value Close(napi_env env, napi_callback_info info) {
        napi_value thisArgs;
        NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisArgs, nullptr))
        NapiWrapper *wrapper = GetWrapper(env, thisArgs);
        if (wrapper != nullptr) {
            wrapper->GetNative()->Close();
            RemoveWrapper(wrapper->GetNative());
        }
        return nullptr;
    }
};

#endif // JEMOC_STREAM_TEST_TEXTREADERBINDING_H
