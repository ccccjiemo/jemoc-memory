//
// Created on 2025/2/18.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "stream/BrotliStream.h"
#include <cstdint>
#include <memory>

napi_value BrotliJs::JSDecompress(napi_env env, napi_callback_info info) {
    napi_value argv[1]{nullptr};
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < 1) {
        napi_throw_type_error(env, "BrotliUtils", "invalid argument");
        return nullptr;
    }

    auto buffer = GetBuffer(env, argv[0]);

    return JSDecompressCore(env, buffer.first.get(), buffer.second);
}

napi_value BrotliJs::JSCompress(napi_env env, napi_callback_info info) {
    napi_value argv[2]{nullptr};
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < 1) {
        napi_throw_type_error(env, "BrotliUtils", "invalid argument");
        return nullptr;
    }

    auto buffer = GetBuffer(env, argv[0]);

    BrotliConfig config{.quality = BROTLI_DEFAULT_QUALITY, .lgWin = BROTLI_DEFAULT_WINDOW, .mode = BROTLI_DEFAULT_MODE};

    if (argc == 2) {
        getBrotliConfig(env, argv[1], config);
    }

    return JSCompressCore(env, buffer.first.get(), buffer.second, config);
}

napi_value BrotliJs::JSDecompressCore(napi_env env, void *buffer, size_t length) {
    std::vector<uint8_t> decompressData = BrotliDecoder::tryDecompress(static_cast<uint8_t *>(buffer), length);
    napi_value result = nullptr;
    void *decompress_data = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, decompressData.size(), &decompress_data, &result));
    memcpy(decompress_data, decompressData.data(), decompressData.size());
    decompressData.clear();
    return result;
}

napi_value BrotliJs::JSCompressCore(napi_env env, void *buffer, size_t length, const BrotliConfig &config) {
    size_t compressedLength = BrotliEncoder::getMaxCompressedLength(length);
    uint8_t *compressData = new uint8_t[compressedLength];
    Buffer source(buffer, 0, length);
    Buffer destination(compressData, 0, compressedLength);
    size_t availableOutput = 0;
    bool encodeResult = BrotliEncoder::tryCompress(source, destination, availableOutput, config);
    napi_value result = nullptr;

    if (encodeResult) {
        void *resultData = nullptr;
        NAPI_CALL(env, napi_create_arraybuffer(env, availableOutput, &resultData, &result))
        memcpy(resultData, compressData, availableOutput);
    }

    delete[] compressData;
    return result;
}

void BrotliJs::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"compress", nullptr, JSCompress, nullptr, nullptr, nullptr, napi_static, nullptr},
        {"decompress", nullptr, JSDecompress, nullptr, nullptr, nullptr, napi_static, nullptr},
        {"compressAsync", nullptr, JSCompressAsync, nullptr, nullptr, nullptr, napi_static, nullptr},
        {"decompressAsync", nullptr, JSDecompressAsync, nullptr, nullptr, nullptr, napi_static, nullptr}
        
    };
    napi_value cons;
    NAPI_CALL(env, napi_define_class(
                       env, "BrotliUtils", NAPI_AUTO_LENGTH,
                       [](napi_env env, napi_callback_info info) -> napi_value { return nullptr; }, nullptr,
                       sizeof(desc) / sizeof(desc[0]), desc, &cons))
    NAPI_CALL(env, napi_set_named_property(env, exports, "BrotliUtils", cons))
}

std::pair<std::shared_ptr<uint8_t>, size_t> BrotliJs::GetBuffer(napi_env env, napi_value value) {
    napi_valuetype type;
    size_t length = 0;
    NAPI_CALL(env, napi_typeof(env, value, &type))
    if (type == napi_string) {
        NAPI_CALL(env, napi_get_value_string_utf8(env, value, nullptr, 0, &length))
        auto ptr = std::shared_ptr<uint8_t>(new uint8_t[length + 1], [](uint8_t *p) { delete[] p; });
        NAPI_CALL(env, napi_get_value_string_utf8(env, value, (char *)ptr.get(), length + 1, &length))
        return {ptr, length};

    } else {
        void *t = nullptr;
        getBuffer(env, value, &t, &length);
        auto ptr = std::shared_ptr<uint8_t>(reinterpret_cast<uint8_t *>(t), [](uint8_t *p) {});
        return {ptr, length};
    }
}

napi_value BrotliJs::JSCompressAsync(napi_env env, napi_callback_info info) {
    napi_value argv[2]{nullptr};
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < 1) {
        napi_throw_type_error(env, "BrotliUtils", "invalid argument");
        return nullptr;
    }
    auto buffer = GetBuffer(env, argv[0]);
    BrotliConfig config{.quality = BROTLI_DEFAULT_QUALITY, .lgWin = BROTLI_DEFAULT_WINDOW, .mode = BROTLI_DEFAULT_MODE};
    if (argc == 2) {
        getBrotliConfig(env, argv[1], config);
    }

    AsyncData *data = new AsyncData{.buffer = buffer, .config = config};

    napi_value resourceName = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "compressAsync", NAPI_AUTO_LENGTH, &resourceName))
    NAPI_CALL(env, napi_create_promise(env, &data->deferred, &promise))
    NAPI_CALL(env, napi_create_async_work(
                       env, nullptr, resourceName,
                       [](napi_env env, void *data) {
                           AsyncData *asyncData = (AsyncData *)data;
                           asyncData->result = JSCompressCore(env, asyncData->buffer.first.get(),
                                                              asyncData->buffer.second, asyncData->config);
                       },
                       [](napi_env env, napi_status status, void *data) {
                           AsyncData *asyncData = (AsyncData *)data;
                           if (status != napi_ok || asyncData->result == nullptr) {
                               NAPI_CALL(env, napi_reject_deferred(env, asyncData->deferred, nullptr))
                           } else {
                               NAPI_CALL(env, napi_resolve_deferred(env, asyncData->deferred, asyncData->result))
                           }
                           NAPI_CALL(env, napi_delete_async_work(env, asyncData->work))
                           delete asyncData;
                       },
                       data, &data->work))
    NAPI_CALL(env, napi_queue_async_work(env, data->work))

    return promise;
}

napi_value BrotliJs::JSDecompressAsync(napi_env env, napi_callback_info info) {
    napi_value argv[1]{nullptr};
    size_t argc = 1;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < 1) {
        napi_throw_type_error(env, "BrotliUtils", "invalid argument");
        return nullptr;
    }

    auto buffer = GetBuffer(env, argv[0]);

    AsyncData *data = new AsyncData{.buffer = buffer};

    napi_value resourceName = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "decompressAsync", NAPI_AUTO_LENGTH, &resourceName))
    NAPI_CALL(env, napi_create_promise(env, &data->deferred, &promise))
    NAPI_CALL(env, napi_create_async_work(
                       env, nullptr, resourceName,
                       [](napi_env env, void *data) {
                           AsyncData *asyncData = (AsyncData *)data;
                           asyncData->result =
                               JSDecompressCore(env, asyncData->buffer.first.get(), asyncData->buffer.second);
                       },
                       [](napi_env env, napi_status status, void *data) {
                           AsyncData *asyncData = (AsyncData *)data;
                           if (status != napi_ok || asyncData->result == nullptr) {
                               NAPI_CALL(env, napi_reject_deferred(env, asyncData->deferred, nullptr))
                           } else {
                               NAPI_CALL(env, napi_resolve_deferred(env, asyncData->deferred, asyncData->result))
                           }
                           NAPI_CALL(env, napi_delete_async_work(env, asyncData->work))
                           delete asyncData;
                       },
                       data, &data->work))
    NAPI_CALL(env, napi_queue_async_work(env, data->work))

    return promise;
}
