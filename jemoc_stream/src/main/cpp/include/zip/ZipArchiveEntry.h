//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H
#define JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H

#include "stream/MemoryStream.h"
#include "zip/ZipRecord.h"
#include <cstdint>
#include <napi/native_api.h>
#include <string>
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
    ZipArchiveEntry(ZipArchive *archive, const ZipCentralDirectoryRecord &record);
    ZipArchiveEntry(ZipArchive *archive, const std::string &entryName, int compressionLevel);
    ~ZipArchiveEntry();

    std::shared_ptr<IStream> open();
    bool getIsEncrypted() const;
    void setIsEncrypted(const bool &value);
    CompressionLevel getCompressionLevel() const;
    void setCompressionLevel(CompressionLevel level);
    bool getHasDataDescriptor() const;
    void setHasDataDescriptor(bool value);
    std::string getComment() const;
    void setComment(const std::string &comment);
    std::string getFullName();
    void setFullName(const std::string &entryName);
    CompressionMethod getCompressionMethod() const;
    void setCompressionMethod(CompressionMethod value);
    double getLastModifier() const;
    void setLastModifier(double value);

public:
    long getOffsetOfCompressedData();
    bool writeLocalFileHeader();
    void writeCrcAndSizesInLocalHeader();
    void writeDataDescriptor();
    void writeAndFinishLocalEntry();
    void writeCentralDirectoryFileHeader();
    void loadLocalHeaderExtraFieldAndCompressedBytesIfNeeded();
    void writeLocalFileHeaderAndDataIfNeeded();
    uint getCryptCRC() const;
    ZipArchive *getArchive();

private:
    std::shared_ptr<IStream> openInReadMode();
    std::shared_ptr<IStream> openInCreateMode();
    std::shared_ptr<IStream> openInUpdateMode();
    std::shared_ptr<IStream> getDataDecompressor(std::shared_ptr<IStream> stream);
    std::shared_ptr<IStream> getDataCompressor(std::shared_ptr<IStream> stream, bool leaveOpen);
    std::shared_ptr<IStream> getUncompressedData();
    void closeStream();
    void Delete();


public:
    static std::string ClassName;
    static napi_ref cons;
    static void Export(napi_env env, napi_value exports);
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static ZipArchiveEntry *getEntry(napi_env env, napi_value value);
    napi_value getJSEntry(napi_env env);
    void releaseJSEntry(napi_env env);
    napi_value open(napi_env env);
    napi_ref jsEntry = nullptr;
    void Delete(napi_env env);
    
    
//     void setIsEncrypted(const bool &value);
//     CompressionLevel getCompressionLevel() const;
//     void setCompressionLevel(CompressionLevel level);
//     bool getHasDataDescriptor() const;
//     void setHasDataDescriptor(bool value);
//     std::string getComment() const;
//     void setComment(const std::string &comment);
//     std::string getFullName();
//     void setFullName(const std::string &entryName);
//     CompressionMethod getCompressionMethod() const;
//     void setCompressionMethod(CompressionMethod value);
//     double getLastModifier() const ;
//     void setLastModifier(double value);
    static napi_value JSOpen(napi_env env, napi_callback_info info);
    static napi_value JSSetIsEncrypted(napi_env env, napi_callback_info info);
    static napi_value JSGetIsEncrypted(napi_env env, napi_callback_info info);
    static napi_value JSGetCompressionLevel(napi_env env, napi_callback_info info);
    static napi_value JSSetCompressionLevel(napi_env env, napi_callback_info info);
    static napi_value JSSetFileComment(napi_env env, napi_callback_info info);
    static napi_value JSGetFileComment(napi_env env, napi_callback_info info);
    static napi_value JSGetFullName(napi_env env, napi_callback_info info);
    static napi_value JSSetFullName(napi_env env, napi_callback_info info);
    static napi_value JSGetCompressionMethod(napi_env env, napi_callback_info info);
    static napi_value JSSetCompressionMethod(napi_env env, napi_callback_info info);
    static napi_value JSSetLastModifier(napi_env env, napi_callback_info info);
    static napi_value JSGetLastModifier(napi_env env, napi_callback_info info);
    static napi_value JSGetIsOpened(napi_env env, napi_callback_info info);
    static napi_value JSGetCRC(napi_env env, napi_callback_info info);
    static napi_value JSDelete(napi_env env, napi_callback_info info);
    static napi_value JSGetIsDeleted(napi_env env, napi_callback_info info);
    static napi_value JSGetUnCompressedSize(napi_env env, napi_callback_info info);
    static napi_value JSGetCompressedSize(napi_env env, napi_callback_info info);

private:
    IStream *openingStream = nullptr;

    bool isUnicodeFileName = false;

    int m_compression_level;
    ZipArchive *m_archive;
    bool isEncrypted;
    bool m_originallyInArchive;
    ushort diskNumberStart;

    ushort versionMadeBy;
    ushort versionToExtract;
    ushort flags;
    ushort compressionMethod;
    uint lastModifier;
    uint crc;
    uint compressedSize;
    uint uncompressedSize;
    uint externalFileAttr;
    uint headerOffset;

    long stored_offsetOfCompressedData = -1;

    ushort fileNameLength = 0;
    ushort fileCommentLength = 0;
    ushort extraFieldLength = 0;

    char *fileName = nullptr;
    char *fileComment = nullptr;
    std::vector<ZipGenericExtraField *> fields;

    std::string m_stored_fullname = "";

    bool m_everOpenedForWrite = false;
    bool m_currentlyOpenForWrite = false;

    IStream *m_outstandingWriteStream = nullptr;
    std::shared_ptr<MemoryStream> compressedBytes = nullptr;
    std::shared_ptr<MemoryStream> uncompressedData = nullptr;

    byte *cdExtraFields = nullptr;
    byte *lfExtraFields = nullptr;
    ushort lfExtraFieldsLength = 0;
};

#endif // JEMOC_STREAM_TEST_ZIPARCHIVEENTRY_H
