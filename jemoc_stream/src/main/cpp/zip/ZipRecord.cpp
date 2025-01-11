//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipRecord.h"
#include <cstdint>

bool ZipEndOfCentralDirectoryRecord::tryReadRecord(IStream *stream, ZipEndOfCentralDirectoryRecord *record) {
    ZipEndOfCentralDirectoryRecord _record;
    stream->read(&record, 0, sizeof(ZipEndOfCentralDirectoryRecord));
    if (_record.signature == ZIP_EOCD_SIGNATURE) {
        *record = _record;
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

ZipCentralDirectoryRecord::~ZipCentralDirectoryRecord() {
    if (fileName != nullptr) {
        delete[] fileName;
        fileName = nullptr;
    }
    if (fileComment != nullptr) {
        delete[] fileComment;
        fileComment = nullptr;
    }
    if (extraField != nullptr) {
        delete[] extraField;
        extraField = nullptr;
    }
}

bool ZipCentralDirectoryRecord::tryReadRecord(IStream *stream, bool saveExtraFieldsAndComment,
                                              ZipCentralDirectoryRecord *record) {
    ZipCentralDirectoryRecord *_record = new ZipCentralDirectoryRecord;
    stream->read(record, 0, ZIP_SIZEOF_CentralDirectory_Header);
    if (_record->signature != ZIP_CentralDirectory_SIGNATURE) {
        delete _record;
        return false;
    }

    if (_record->fileNameLength) {
        _record->fileName = new char[_record->fileNameLength];
        stream->read(_record->fileName, 0, _record->fileNameLength);
    }
    if (_record->extraFieldLength) {
        _record->extraField = new char[_record->extraFieldLength];
        stream->read(_record->extraField, 0, _record->extraFieldLength);
    }
    if (_record->fileCommentLength) {
        _record->fileComment = new char[_record->fileCommentLength];
        stream->read(_record->fileComment, 0, _record->fileCommentLength);
    }
    *record = *_record;
    return true;
}

std::vector<ZipGenericExtraField> ZipGenericExtraField::tryRead(void *buffer, size_t size) {
    std::vector<ZipGenericExtraField> list;
    long pointer = 0;
    while (pointer + 4 < size) {
        ZipGenericExtraField field;
        memcpy(&field, offset_pointer(buffer, pointer), 4);
        pointer += 4;
        if (field.size > size - pointer)
            break;
        field.data = new uint8_t[field.size];
        memcpy(field.data, offset_pointer(buffer, pointer), field.size);
        pointer += field.size;
        list.push_back(field);
    }
    return list;
}