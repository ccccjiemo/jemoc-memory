#include "BufferPool.h"
#include "binding/StreamReaderBinding.h"
#include "binding/TextReaderBinding.h"
#include "binding/XmlReaderBinding.h"
#include "napi/native_api.h"
#include "stream/BrotliStream.h"
#include "stream/DeflateStream.h"
#include "stream/FileStream.h"
#include "stream/MemfdStream.h"
#include "stream/MemoryStream.h"
#include "zip/ZipArchive.h"
#include "zip/ZipArchiveEntry.h"
#include "zip/ZipCryptoStream.h"

DECLARE_ROOT_START(ReaderModule)
DECLARE_NAMESPACE_START(reader)
DECLARE_CLASS(TextReaderBinding)
DECLARE_CLASS(StreamReaderBinding)
DECLARE_CLASS(XmlReaderBinding)
DECLARE_NAMESPACE_END
DECLARE_ROOT_END


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    IStream::Export(env, exports);
    MemfdStream::Export(env, exports);
    MemoryStream::Export(env, exports);
    FileStream::Export(env, exports);
    DeflateStream::Export(env, exports);
    ZipCryptoStream::Export(env, exports);
    ZipArchive::Export(env, exports);
    ZipArchiveEntry::Export(env, exports);
    Inflater::Export(env, exports);
    Deflater::Export(env, exports);
    BrotliStream::Export(env, exports);
    BrotliJs::Export(env, exports);
    jemoc_stream::BufferPool::Export(env, exports);
    jemoc_stream::LruBufferPool::Export(env, exports);

   

    napi_value reader = JSBindingTool::ExportNamespace(env, exports, "reader");
    TextReaderBinding::ExportTopLevel(env, reader);
    StreamReaderBinding::ExportTopLevel(env, reader);
    XmlReaderBinding::ExportTopLevel(env, reader);

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
