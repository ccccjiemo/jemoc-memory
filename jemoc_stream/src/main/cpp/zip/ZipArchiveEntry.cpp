//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipArchiveEntry.h"
#include "zip/ZipArchive.h"
#include "zip/ZipRecord.h"

static ushort mapDeflateCompressionOption(ushort flag, CompressionLevel compressionLevel,
                                          CompressionMethod compressionMethod) {}

ZipArchiveEntry::ZipArchiveEntry(ZipArchive *archive, ZipCentralDirectoryRecord *record) {
    m_archive = archive;
    m_originallyInArchive = true;
    diskNumberStart = record->diskNumberStart;
    verionMadeBy = record->versionMadeBy;
    versionToExtract = record->versionToExtract;
    flags = record->flags;
    isEncrypted = (flags & GeneralPurposeBitFlag_IsEncrypted) != 0;
    compressionMethod = record->compression;
    fileTime = record->fileTime;
    fileDate = record->fileDate;
    compressedSize = record->compressedSize;
    uncompressedSize = record->uncompressedSize;
    externalFileAttr = record->externalAttributes;
    headerOffset = record->headerOffset;
    crc = record->crc;
    fileName = std::string(record->fileName);
    fileComment = std::string(record->fileComment);
    fields = ZipGenericExtraField::tryRead(record->extraField, record->extraFieldLength);
}


ZipArchiveEntry::ZipArchiveEntry(ZipArchive *archive, const std::string &entryName, int compressionLevel) {
    m_compression_level = compressionLevel;
    m_archive = archive;
    if (m_compression_level == CompressionLevel_NoCompression) {
        compressionMethod = CompressionMethod::Stored;
    }
}


#ifndef DEFINE_ZipArchiveEntry_NAPI
#define DEFINE_ZipArchiveEntry_NAPI
#define GET_ZIPARCHIVE_ENTRY_INFO(number)                                                                              \
    napi_value argv[number];                                                                                           \
    size_t argc = number;                                                                                              \
    napi_value _this = nullptr;                                                                                        \
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &_this, nullptr))

#define GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(number)                                                                   \
    GET_ZIPARCHIVE_ENTRY_INFO(number)                                                                                  \
    ZipArchiveEntry *entry = getEntry(env, _this);                                                                     \
    if (entry == nullptr)                                                                                              \
        napi_throw_error(env, "ZipArchiveEntry", "entry is null");


std::string ZipArchiveEntry::ClassName = "ZipArchiveEntry";

napi_ref ZipArchiveEntry::cons = nullptr;

void ZipArchiveEntry::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {

    };
    napi_value napi_cons = nullptr;
    NAPI_CALL(env, napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr,
                                     sizeof(desc) / sizeof(desc[0]), desc, &napi_cons))
    NAPI_CALL(env, napi_set_named_property(env, exports, ClassName.c_str(), napi_cons))
    NAPI_CALL(env, napi_create_reference(env, napi_cons, 1, &cons))
}

napi_value ZipArchiveEntry::JSConstructor(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO(0)
    return _this;
}

napi_value ZipArchiveEntry::createJSEntry(napi_env env, ZipArchiveEntry *entry, napi_ref *ref) {
    napi_value napi_cons = nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, cons, &napi_cons))
    NAPI_CALL(env, napi_new_instance(env, napi_cons, 0, nullptr, &result))
    napi_wrap(env, result, entry, JSDispose, nullptr, ref);
    return result;
}

ZipArchiveEntry *ZipArchiveEntry::getEntry(napi_env env, napi_value value) {
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, value, &type))
    if (napi_undefined == type)
        return nullptr;
    void *result = nullptr;
    napi_unwrap(env, value, &result);
    return static_cast<ZipArchiveEntry *>(result);
}


#endif // DEFINE_ZipArchiveEntry_NAPI
