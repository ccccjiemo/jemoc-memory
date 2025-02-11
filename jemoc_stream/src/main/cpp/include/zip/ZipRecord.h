//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPENDOFCENTRALDIRECTORYRECORD_H
#define JEMOC_STREAM_TEST_ZIPENDOFCENTRALDIRECTORYRECORD_H

#include <sys/types.h>
#include "IStream.h"

#define ZIP_EOCD_SIGNATURE 0x06054b50
#define ZIP_CentralDirectory_SIGNATURE 0x02014b50
#define ZIP_DATADESCRIPTOR_SIGNATURE 0x08074b50
#define ZIP_LOCALFILEHEADER_SIGNATURE 0x04034b50
#define ZIP_SIZEOF_CentralDirectory_Header 46
#define ZIP_EOCD_SIZEOFRECORD_WITHOUT_SIGNATURE 18
#define ZIP_LOCALFILEHEADER_OFFSET_TO_CRC 14

struct ZipEndOfCentralDirectoryRecord {
    uint signature;
    ushort diskNumber;
    ushort startDiskNumber;
    ushort entriesOnDisk;
    ushort entriesInDirectory;
    uint directorySize;
    uint directoryOffset;
    ushort commentLength;
    static bool tryReadRecord(IStream *stream, ZipEndOfCentralDirectoryRecord *record);
    static void writeRecord(IStream *stream, ushort entriesOnDisk, uint directoryOffset, uint sizeOfDirectory,
                            std::string comment);
} __attribute__((packed));

struct ZipCentralDirectoryRecord {
    uint signature;
    ushort versionMadeBy;
    ushort versionToExtract;
    ushort flags;
    ushort compression;
    uint lastModifier;
    uint crc;
    uint compressedSize;
    uint uncompressedSize;
    ushort fileNameLength;
    ushort extraFieldLength;
    ushort fileCommentLength;
    ushort diskNumberStart;
    ushort internalAttributes;
    uint externalAttributes;
    uint headerOffset;
    static bool tryReadRecord(IStream *stream, bool saveExtraFieldsAndComment, ZipCentralDirectoryRecord *record);
} __attribute__((packed));

struct ZipGenericExtraField {
    ushort tag;
    ushort size;
    uint8_t *data = nullptr;
    ~ZipGenericExtraField();
    static std::vector<ZipGenericExtraField *> tryRead(void *buffer, size_t size);
} __attribute__((packed));

struct ZipDataDescriptor {
    uint signature;
    uint crc;
    uint compressedSize;
    uint uncompressedSize;
    static bool tryRead(IStream *stream, ZipDataDescriptor *descriptor);
};

struct ZipLocalFileHeader {
    uint signature;
    ushort version;
    ushort flags;
    ushort compression;
    uint lastModifier;
    uint crc;
    uint compressedSize;
    uint uncompressedSize;
    ushort fileNameLength;
    ushort extraFieldLength;
    static bool trySkip(IStream *stream);
} __attribute__((packed));

struct InfoZIPUnicodeCommentExtraField {
    byte version;
    uint crc32;
    std::string data;
    static bool tryRead(ZipGenericExtraField *field, InfoZIPUnicodeCommentExtraField *result);
};

namespace ZipCentralDirectory {}
#endif // JEMOC_STREAM_TEST_ZIPENDOFCENTRALDIRECTORYRECORD_H
