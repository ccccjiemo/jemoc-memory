//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipArchiveEntry.h"
#include "stream/DeflateStream.h"
#include "stream/SubReadStream.h"
#include "zip/DirectToArchiveWriterStream.h"
#include "zip/WrappedStream.h"
#include "zip/ZipArchive.h"
#include "zip/ZipRecord.h"
#include "zip/ZipCryptoStream.h"
#include "stream/DeflateStream.h"
#include "zip/CheckSumAndSizeWriteStream.h"

static ushort mapCompressionLevel(ushort flag, ushort compressionMethod) {
    if (compressionMethod == CompressionMethod::Deflate || compressionMethod == CompressionMethod::Deflate64) {
        ushort level = flag & 0x6;
        switch (level) {
        case 0:
            return CompressionLevel_Optimal;
        case 2:
            return CompressionLevel_SmallestSize;
        case 4:
        case 6:
            return CompressionLevel_Fastest;
        default:
            return CompressionLevel_Optimal;
        }
    }
    return CompressionLevel_NoCompression;
}

static int getZlibCompressionLevel(CompressionLevel level) {
    switch (level) {
    case CompressionLevel_Optimal:
        return Z_DEFAULT_COMPRESSION;
    case CompressionLevel_Fastest:
        return Z_BEST_SPEED;
    case CompressionLevel_NoCompression:
        return Z_NO_COMPRESSION;
    case CompressionLevel_SmallestSize:
        return Z_BEST_COMPRESSION;
    default:
        return Z_DEFAULT_COMPRESSION;
    }
}

ZipArchiveEntry::ZipArchiveEntry(ZipArchive *archive, const ZipCentralDirectoryRecord &record) {
    m_archive = archive;
    m_originallyInArchive = true;
    diskNumberStart = record.diskNumberStart;
    versionMadeBy = record.versionMadeBy;
    versionToExtract = record.versionToExtract;
    flags = record.flags;
    isEncrypted = (flags & GeneralPurposeBitFlag_IsEncrypted) != 0;
    compressionMethod = record.compression;
    fileTime = record.fileTime;
    fileDate = record.fileDate;
    compressedSize = record.compressedSize;
    uncompressedSize = record.uncompressedSize;
    externalFileAttr = record.externalAttributes;
    headerOffset = record.headerOffset;
    crc = record.crc;
    m_compression_level = mapCompressionLevel(flags, compressionMethod);

    fileName = new char[record.fileNameLength];
    archive->getArchiveStream()->read(fileName, 0, record.fileNameLength);
    fields = ZipGenericExtraField::tryRead(archive->getArchiveStream(), record.extraFieldLength);
    if (record.fileCommentLength > 0) {
        fileComment = new char[record.fileCommentLength];
        archive->getArchiveStream()->read(fileComment, 0, record.fileCommentLength);
    }

    if (flags & GeneralPurposeBitFlag_DataDescriptor) {
        ZipDataDescriptor descriptor;
        if (ZipDataDescriptor::tryRead(archive->getArchiveStream(), &descriptor)) {
            crc = descriptor.crc;
            compressedSize = descriptor.compressedSize;
            uncompressedSize = descriptor.uncompressedSize;
        }
    }
}


ZipArchiveEntry::ZipArchiveEntry(ZipArchive *archive, const std::string &entryName, int compressionLevel) {
    m_compression_level = compressionLevel;
    m_archive = archive;
    if (m_compression_level == CompressionLevel_NoCompression) {
        compressionMethod = CompressionMethod::Stored;
    }
    diskNumberStart = 0;
    versionMadeBy = 20;
    versionToExtract = 20;
    flags = 0;
    isEncrypted = (flags & GeneralPurposeBitFlag_IsEncrypted) != 0;
    compressionMethod = CompressionMethod::Stored;
    fileTime = 0;
    fileDate = 0;
    compressedSize = 0;
    uncompressedSize = 0;
    externalFileAttr = 0;
    headerOffset = 0;
    setFullName(entryName);
//     fileName = (char *)entryName.c_str();
//     fileNameLength = entryName.length();
    fileCommentLength = 0;
    crc = 0;
    m_originallyInArchive = false;
    m_stored_fullname = entryName;
}

std::string ZipArchiveEntry::getFullName() {
    if (m_stored_fullname.length() == 0) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            if (it->tag == 0x7075) {
                m_stored_fullname = std::string((char *)it->data);
                break;
            }
        }
        m_stored_fullname = std::string(fileName);
    }
    return m_stored_fullname;
}

long ZipArchiveEntry::getOffsetOfCompressedData() {
    if (stored_offsetOfCompressedData == -1) {
        IStream *stream = m_archive->getArchiveStream();
        stream->seek(headerOffset, SeekOrigin::Begin);
        if (!ZipLocalFileHeader::trySkip(stream))
            throw std::ios::failure("a local file header is corrupt.");

        stored_offsetOfCompressedData = stream->getPosition();
    }
    return stored_offsetOfCompressedData;
}


IStream *ZipArchiveEntry::open() {
    switch (m_archive->getMode()) {
    case ZipArchiveMode_Read:
        return openInReadMode();
    case ZipArchiveMode_Create:
        return openInCreateMode();
    case ZipArchiveMode_Update:
        return openInUpdateMode();
    }
}

void ZipArchiveEntry::setFullName(const std::string &entryName) {
    m_stored_fullname = entryName;
    fileNameLength = m_stored_fullname.length();
    fileName = (char *)m_stored_fullname.c_str();
}

IStream *ZipArchiveEntry::openInReadMode() {
    IStream *stream =
        new SubReadStream(m_archive->getArchiveStream(), getOffsetOfCompressedData(), compressedSize, true);
    if (flags & GeneralPurposeBitFlag_IsEncrypted) {
        stream = new ZipCryptoStream(stream, CryptoMode_Decode, m_archive->getPassword(), false, crc);
    }
    return getDataDecompressor(stream);
}

IStream *ZipArchiveEntry::openInUpdateMode() {
    if (m_currentlyOpenForWrite)
        throw std::ios::failure("entries cannot be opened multiple times in update mode.");

    m_everOpenedForWrite = true;
    m_currentlyOpenForWrite = true;
    IStream *stream = getUncompressedData();
    stream->seek(0, SeekOrigin::Begin);
    return new WrappedStream(stream, true, [this]() { this->m_currentlyOpenForWrite = false; });
}

IStream *ZipArchiveEntry::getUncompressedData() {
    if (uncompressedData == nullptr) {
        uncompressedData = new MemoryStream(uncompressedSize);
        if (m_originallyInArchive) {
            try {
                IStream *stream = openInReadMode();
                stream->copyTo(uncompressedData, 4096);
                stream->close();
            } catch (const std::exception &e) {
                uncompressedData->close();
                uncompressedData = nullptr;
                m_currentlyOpenForWrite = false;
                m_everOpenedForWrite = false;
                throw std::ios::failure("open failed");
            }
        }
    }
    return uncompressedData;
}


IStream *ZipArchiveEntry::getDataDecompressor(IStream *stream) {
    if (compressionMethod == CompressionMethod::Deflate || compressionMethod == CompressionMethod::Deflate64) {
        return new DeflateStream(stream, DeflateMode_Decompress, -15, m_compression_level, false, 4096,
                                 uncompressedSize);
    }
    return stream;
}

IStream *ZipArchiveEntry::getDataCompressor(IStream *stream, bool leaveOpen) {
    IStream *compressorStream = stream;
    ZipCryptoStream *cryptoStream = nullptr;
    bool isBase = true;
    if (flags & GeneralPurposeBitFlag_IsEncrypted) {
        compressorStream = new ZipCryptoStream(stream, CryptoMode_Encode, m_archive->getPassword(), leaveOpen, 0, 4096);
        isBase = false;
    }
    switch (compressionMethod) {
    case CompressionMethod::Stored:
        compressorStream = stream;
        break;
    case CompressionMethod::Deflate:
    case CompressionMethod::Deflate64:
    default:
        compressorStream = new DeflateStream(stream, DeflateMode_Compress, -15,
                                             getZlibCompressionLevel(CompressionLevel(m_compression_level)),
                                             isBase ? leaveOpen && true : false);
        isBase = false;
        break;
    }

    CheckSumAndSizeWriteStream *checkSumStream =
        new CheckSumAndSizeWriteStream(compressorStream, stream, isBase ? leaveOpen && true : false,
                                       [this, cryptoStream](long initialPosition, long currentPosition, uint checkSum) {
                                           if (cryptoStream != nullptr) {
                                               cryptoStream->setCRC(checkSum);
                                           }
                                           crc = checkSum;
                                           uncompressedSize = currentPosition;
                                           compressedSize =
                                               m_archive->getArchiveStream()->getPosition() - initialPosition;
                                       });

    return checkSumStream;
}

IStream *ZipArchiveEntry::openInCreateMode() {
    if (m_everOpenedForWrite)
        throw std::ios::failure(
            "entries in create mode may only be written to once, and only one entry may be held open at a time.");
    m_everOpenedForWrite = true;
    CheckSumAndSizeWriteStream *crcStream =
        (CheckSumAndSizeWriteStream *)getDataCompressor(m_archive->getArchiveStream(), true);
    return new DirectToArchiveWriterStream(crcStream, this);
}

CompressionMethod ZipArchiveEntry::getCompressionMethod() const { return CompressionMethod(compressionMethod); }

bool ZipArchiveEntry::writeLocalFileHeader() {
    headerOffset = m_archive->getArchiveStream()->getPosition();
    ZipLocalFileHeader header;
    header.signature = ZIP_LOCALFILEHEADER_SIGNATURE;
    header.version = versionToExtract;
    header.flags = flags;
    header.compression = uncompressedSize == 0 ? CompressionLevel_NoCompression : compressionMethod;
    header.lastModifier = fileTime | fileDate << 16;
    header.crc = crc;
    header.compressedSize = compressedSize;
    header.uncompressedSize = uncompressedSize;
    header.fileNameLength = fileNameLength;
    header.extraFieldLength = extraFieldLength;
    IStream *stream = m_archive->getArchiveStream();
    stream->write(&header, 0, sizeof(header));
    stream->write(fileName, 0, fileNameLength);
    if (extraFieldLength > 0) {
        stream->write(extraField, 0, extraFieldLength);
    }
    return true;
}

void ZipArchiveEntry::writeCrcAndSizesInLocalHeader() {
    long finalPosition = m_archive->getArchiveStream()->getPosition();
    IStream *stream = m_archive->getArchiveStream();
    stream->seek(headerOffset + ZIP_LOCALFILEHEADER_OFFSET_TO_CRC, SeekOrigin::Begin);
    stream->write(&crc, 0, sizeof(crc));
    stream->write(&compressedSize, 0, sizeof(compressedSize));
    stream->write(&uncompressedSize, 0, sizeof(uncompressedSize));
    stream->seek(finalPosition, SeekOrigin::Begin);
}

void ZipArchiveEntry::writeDataDescriptor() {
    IStream *stream = m_archive->getArchiveStream();
    stream->write(&crc, 0, sizeof(crc));
    stream->write(&compressedSize, 0, sizeof(compressedSize));
    stream->write(&uncompressedSize, 0, sizeof(uncompressedSize));
}

void ZipArchiveEntry::writeAndFinishLocalEntry() {
    closeStream();
    writeLocalFileHeaderAndDataIfNeeded();
    if (uncompressedData != nullptr) {
        uncompressedData->close();
        delete uncompressedData;
        uncompressedData = nullptr;
    }
    if (compressedBytes != nullptr) {
        compressedBytes->close();
        delete compressedBytes;
        compressedBytes = nullptr;
    }
}

void ZipArchiveEntry::writeLocalFileHeaderAndDataIfNeeded() {
    if (uncompressedData != nullptr) {
        uncompressedSize = uncompressedData->getLength();
        IStream *entryWriter =
            new DirectToArchiveWriterStream(getDataCompressor(m_archive->getArchiveStream(), true), this);
        uncompressedData->seek(0, SeekOrigin::Begin);
        uncompressedData->copyTo(entryWriter, 4096);
        uncompressedData->close();
        uncompressedData = nullptr;
        entryWriter->close();
        delete entryWriter;
        entryWriter = nullptr;
    } else {
        if (uncompressedSize == 0) {
            compressedSize = 0;
        }
        writeLocalFileHeader();

        if (uncompressedSize != 0) {
            m_archive->getArchiveStream()->write((void *)compressedBytes->getData(), 0, compressedBytes->getLength());
            compressedBytes->close();
            delete compressedBytes;
            compressedBytes = nullptr;
        }
    }
}

void ZipArchiveEntry::loadLocalHeaderExtraFieldAndCompressedBytesIfNeeded() {
    if (m_originallyInArchive) {
        m_archive->getArchiveStream()->seek(headerOffset + 30 + fileNameLength, SeekOrigin::Begin);
    }

    if (!m_everOpenedForWrite && m_originallyInArchive) {
        if (compressedBytes != nullptr) {
            compressedBytes->close();
            delete compressedBytes;
        }
        compressedBytes = new MemoryStream(compressedSize);
        SubReadStream *subRead =
            new SubReadStream(m_archive->getArchiveStream(), getOffsetOfCompressedData(), compressedSize, true);
        char *buffer = new char[4096];
        long readBytes = 0;
        while ((readBytes = subRead->read(buffer, 0, 4096)) != 0) {
            compressedBytes->write(buffer, 0, readBytes);
        }
        delete[] buffer;
        subRead->close();
        delete subRead;
    }
}


void ZipArchiveEntry::writeCentralDirectoryFileHeader() {
    ZipCentralDirectoryRecord record{};
    record.signature = ZIP_CentralDirectory_SIGNATURE;
    record.versionMadeBy = versionMadeBy;
    record.versionToExtract = versionToExtract;
    record.flags = flags;
    record.compression = compressionMethod;
    record.fileTime = fileTime;
    record.fileDate = fileDate;
    record.crc = crc;
    record.compressedSize = compressedSize;
    record.uncompressedSize = uncompressedSize;
    record.fileNameLength = fileNameLength;
    record.extraFieldLength = extraFieldLength;
    record.fileCommentLength = fileCommentLength;
    record.diskNumberStart = 0;
    record.internalAttributes = 0;
    record.externalAttributes = externalFileAttr;
    record.headerOffset = headerOffset;
    IStream *stream = m_archive->getArchiveStream();
    stream->write(&record, 0, sizeof(record));
    stream->write(fileName, 0, fileNameLength);
    if (extraFieldLength > 0) {
        stream->write(extraField, 0, extraFieldLength);
    }
    if (fileCommentLength > 0) {
        stream->write(fileComment, 0, fileCommentLength);
    }
}


void ZipArchiveEntry::closeStream() {
    if (m_outstandingWriteStream != nullptr) {
        m_outstandingWriteStream->close();
        m_outstandingWriteStream = nullptr;
    }
}


ZipArchive *ZipArchiveEntry::getArchive() { return m_archive; }


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
        DEFINE_NAPI_FUNCTION("open", JSOpen, nullptr, nullptr, nullptr),
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


ZipArchiveEntry *ZipArchiveEntry::getEntry(napi_env env, napi_value value) {
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, value, &type))
    if (napi_undefined == type)
        return nullptr;
    void *result = nullptr;
    napi_unwrap(env, value, &result);
    return static_cast<ZipArchiveEntry *>(result);
}

void ZipArchiveEntry::JSDispose(napi_env env, void *data, void *hint) {}

napi_value ZipArchiveEntry::open(napi_env env) {
    IStream *stream = open();
    openingStream = stream;
    napi_value result = IStream::JSCreateInterface(env, stream);
//     NAPI_CALL(env, napi_create_reference(env, result, 1, &jsOpeningStream))
    return result;
}

napi_value ZipArchiveEntry::JSOpen(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    try {
        return entry->open(env);
    } catch (const std::exception &e) {
        napi_throw_error(env, "ZipArchiveEntry", e.what());
    }
    return nullptr;
}

napi_value ZipArchiveEntry::getJSEntry(napi_env env) {
    napi_value result = nullptr;
    if (jsEntry != nullptr) {
        NAPI_CALL(env, napi_get_reference_value(env, jsEntry, &result));
    } else {
        napi_value napi_cons = nullptr;
        NAPI_CALL(env, napi_get_reference_value(env, cons, &napi_cons))
        NAPI_CALL(env, napi_new_instance(env, napi_cons, 0, nullptr, &result))
        napi_wrap(env, result, this, JSDispose, nullptr, &jsEntry);
    }
    return result;
}

void ZipArchiveEntry::releaseJSEntry(napi_env env) {
    if (jsEntry == nullptr)
        return;

    napi_value value = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, jsEntry, &value))
    if (value == nullptr)
        return;
    void *_this = nullptr;
    NAPI_CALL(env, napi_remove_wrap(env, value, &_this))
    uint ref_count = 0;
    NAPI_CALL(env, napi_reference_unref(env, jsEntry, &ref_count))
    if (ref_count > 0) {
        NAPI_CALL(env, napi_delete_reference(env, jsEntry))
    }

    jsEntry = nullptr;
}


#endif // DEFINE_ZipArchiveEntry_NAPI
