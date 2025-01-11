//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H
#define JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H

#include "zip/ZipRecord.h"
#include <cstdint>
#include <string>
#include <napi/native_api.h>
#include <sys/types.h>

class ZipArchive;
struct ZipCentralDirectoryRecord;


enum CompressionLevel {
    CompressionLevel_Optimal = 0,
    CompressionLevel_Fastest = 1,
    CompressionLevel_NoCompression = 2,
    CompressionLevel_SmallestSize = 3,
};

enum CompressionMethod { Stored = 0x0, Deflate = 0x8, Deflate64 = 0x9, BZip2 = 0xc, LZMA = 0xe };
enum ZipVersionMadeByPlatform { ZipVersionMadeByPlatform_Windows = 0, UZipVersionMadeByPlatform_nix = 3 };
enum ZipVersionNeed {
    ZipVersionNeed_Default = 10,
    ZipVersionNeed_ExplicitDirectory = 20,
    ZipVersionNeed_Deflate = 20,
    ZipVersionNeed_Deflate64 = 21,
    ZipVersionNeed_Zip64 = 45
};

enum GeneralPurposeBitFlag {
    GeneralPurposeBitFlag_IsEncrypted = 0x1,
    GeneralPurposeBitFlag_DataDescriptor = 0x8,
    GeneralPurposeBitFlag_UnicodeFileNameAndComment = 0x800,
};

class ZipArchiveEntry {
public:
    ZipArchiveEntry(ZipArchive *archive, ZipCentralDirectoryRecord *record);
    ZipArchiveEntry(ZipArchive *archive, const std::string &entryName, int compressionLevel);

    std::string getFullName() const;


public:
    static std::string ClassName;
    static napi_ref cons;
    static void Export(napi_env env, napi_value exports);
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static ZipArchiveEntry *getEntry(napi_env env, napi_value value);
    static napi_value createJSEntry(napi_env env, ZipArchiveEntry *entry, napi_ref *ref);

private:
    int m_compression_level;
    ZipArchive *m_archive;
    bool isEncrypted;
    bool m_originallyInArchive;
    ushort diskNumberStart;

    ushort verionMadeBy;
    ushort versionToExtract;
    ushort flags;
    ushort compressionMethod;
    ushort fileTime;
    ushort fileDate;
    uint crc;
    uint compressedSize;
    uint uncompressedSize;
    uint externalFileAttr;
    uint headerOffset;

    std::string fileName;
    std::string fileComment;
    std::vector<ZipGenericExtraField> fields;
};

#endif // JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H
