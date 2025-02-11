//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipRecord.h"
#include <cstdint>

bool ZipEndOfCentralDirectoryRecord::tryReadRecord(IStream *stream, ZipEndOfCentralDirectoryRecord *record) {
    stream->read(record, 0, sizeof(ZipEndOfCentralDirectoryRecord));
    if (record->signature == ZIP_EOCD_SIGNATURE) {
        return true;
    }
    return false;
}
void ZipEndOfCentralDirectoryRecord::writeRecord(IStream *stream, ushort entriesOnDisk, uint directoryOffset,
                                                 uint sizeOfDirectory, std::string comment) {
    ZipEndOfCentralDirectoryRecord record{.signature = ZIP_EOCD_SIGNATURE,
                                          .diskNumber = 0,
                                          .startDiskNumber = 0,
                                          .entriesOnDisk = entriesOnDisk,
                                          .entriesInDirectory = entriesOnDisk,
                                          .directorySize = sizeOfDirectory,
                                          .directoryOffset = directoryOffset,
                                          .commentLength = (ushort)comment.length()};
    stream->write(&record, 0, sizeof(ZipEndOfCentralDirectoryRecord));
    if (comment.length() > 0) {
        stream->write((void *)comment.c_str(), 0, comment.length());
    }
}

bool ZipCentralDirectoryRecord::tryReadRecord(IStream *stream, bool saveExtraFieldsAndComment,
                                              ZipCentralDirectoryRecord *record) {
    stream->read(record, 0, ZIP_SIZEOF_CentralDirectory_Header);
    if (record->signature != ZIP_CentralDirectory_SIGNATURE) {
        return false;
    }
    return true;
}

std::vector<ZipGenericExtraField *> ZipGenericExtraField::tryRead(void *buffer, size_t size) {
    std::vector<ZipGenericExtraField *> list;
    uint8_t *_buffer = static_cast<uint8_t *>(buffer);
    long pointer = 0;
    while (pointer + 4 < size) {
        ZipGenericExtraField *field = new ZipGenericExtraField{0};
        memcpy(field, _buffer + pointer, 4);
        pointer += 4;
        if (field->size > size - pointer)
            break;
        field->data = new uint8_t[field->size];
        memcpy(field->data, _buffer + pointer, field->size);
        pointer += field->size;
        list.push_back(field);
    }
    return list;
}
ZipGenericExtraField::~ZipGenericExtraField() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}


bool ZipDataDescriptor::tryRead(IStream *stream, ZipDataDescriptor *descriptor) {
    stream->read(descriptor, 0, sizeof(ZipDataDescriptor));
    if (descriptor->signature != ZIP_DATADESCRIPTOR_SIGNATURE) {
        stream->seek(-sizeof(ZipDataDescriptor), SeekOrigin::Current);
        return false;
    }
    return true;
}

bool ZipLocalFileHeader::trySkip(IStream *stream) {
    ZipLocalFileHeader header;
    stream->read(&header, 0, sizeof(ZipLocalFileHeader));
    if (header.signature != ZIP_LOCALFILEHEADER_SIGNATURE)
        return false;
    stream->seek(header.fileNameLength + header.extraFieldLength, SeekOrigin::Current);
    return true;
}

bool InfoZIPUnicodeCommentExtraField::tryRead(ZipGenericExtraField *field, InfoZIPUnicodeCommentExtraField *result) {
    // 不符合标签或者空指针返回
    if (field == nullptr || field->tag != 0x7075)
        return false;
    uint pointer = 0;

    // 读取版本
    if (field->size < 1)
        return false;

    memcpy(&result->version, field->data, 1);
    pointer++;

    // 读取crc32
    if (pointer + 4 > field->size)
        return false;

    memcpy(&result->crc32, field->data + pointer, 4);
    pointer += 4;

    std::string data((char *)(field->data + pointer), field->size - pointer);
    result->data = data;
    return true;
}
