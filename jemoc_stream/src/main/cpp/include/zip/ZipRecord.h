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
#define ZIP_SIZEOF_CentralDirectory_Header 46
#define ZIP_EOCD_SIZEOFRECORD_WITHOUT_SIGNATURE 18

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
    ushort fileTime;
    ushort fileDate;
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
    char *fileName = nullptr;
    char *extraField = nullptr;
    char *fileComment = nullptr;
    ~ZipCentralDirectoryRecord();
    static bool tryReadRecord(IStream *stream, bool saveExtraFieldsAndComment, ZipCentralDirectoryRecord *record);
} __attribute__((packed));

struct ZipGenericExtraField {
    ushort tag;
    ushort size;
    uint8_t* data = nullptr;
    static std::vector<ZipGenericExtraField> tryRead(void* buffer, size_t size);
};



namespace ZipCentralDirectory {}
#endif // JEMOC_STREAM_TEST_ZIPENDOFCENTRALDIRECTORYRECORD_H
