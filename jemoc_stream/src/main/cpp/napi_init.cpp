#include "stream/DeflateStream.h"
#include "stream/FileStream.h"
#include "stream/MemfdStream.h"
#include "stream/MemoryStream.h"
#include "zip/ZipArchiveEntry.h"
#include "zip/ZipCryptoStream.h"
#include "zip/ZipArchive.h"
#include "napi/native_api.h"


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    MemfdStream::Export(env, exports);
    MemoryStream::Export(env, exports);
    FileStream::Export(env, exports);
    DeflateStream::Export(env, exports);
    ZipCryptoStream::Export(env, exports);
    ZipArchive::Export(env, exports);
    ZipArchiveEntry::Export(env, exports);
    Inflater::Export(env, exports);
    Deflater::Export(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "jemoc_stream",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterJemoc_streamModule(void) { napi_module_register(&demoModule); }
