//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipArchiveEntry.h"
#include "stream/DeflateStream.h"
#include "stream/SubReadStream.h"
#include "zip/CheckSumAndSizeWriteStream.h"
#include "zip/DirectToArchiveWriterStream.h"
#include "zip/WrappedStream.h"
#include "zip/ZipArchive.h"
#include "zip/ZipCryptoStream.h"
#include "zip/ZipRecord.h"


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
    lastModifier = record.lastModifier;
    compressedSize = record.compressedSize;
    uncompressedSize = record.uncompressedSize;
    externalFileAttr = record.externalAttributes;
    headerOffset = record.headerOffset;
    crc = record.crc;
    m_compression_level = mapCompressionLevel(flags, compressionMethod);

    fileName = new char[record.fileNameLength + 1]{'\0'};
    fileNameLength = record.fileNameLength;
    extraFieldLength = record.extraFieldLength;
    fileCommentLength = record.fileCommentLength;
    archive->getArchiveStream()->read(fileName, 0, record.fileNameLength);
    if (record.extraFieldLength > 0) {
        cdExtraFields = new byte[record.extraFieldLength];
        archive->getArchiveStream()->read(cdExtraFields, 0, record.extraFieldLength);
        fields = ZipGenericExtraField::tryRead(cdExtraFields, record.extraFieldLength);
    }
    if (record.fileCommentLength > 0) {
        fileComment = new char[record.fileCommentLength + 1]{'\0'};
        archive->getArchiveStream()->read(fileComment, 0, record.fileCommentLength);
    }
    if (flags & GeneralPurposeBitFlag_DataDescriptor) {
        ZipDataDescriptor descriptor;
        if (ZipDataDescriptor::tryRead(archive->getArchiveStream().get(), &descriptor)) {
            crc = descriptor.crc;
            compressedSize = descriptor.compressedSize;
            uncompressedSize = descriptor.uncompressedSize;
        }
    }
}


ZipArchiveEntry::ZipArchiveEntry(ZipArchive *archive, const std::string &entryName, int compressionLevel) {
    m_compression_level = compressionLevel;
    m_archive = archive;

    compressionMethod =
        m_compression_level == CompressionLevel_NoCompression ? CompressionMethod::Stored : CompressionMethod::Deflate;
    diskNumberStart = 0;
    versionMadeBy = 20 | 3 << 8;
    versionToExtract = 20;
    flags = 0;
    lastModifier = 0;
    compressedSize = 0;
    uncompressedSize = 0;
    externalFileAttr = 0;
    headerOffset = 0;
    setFullName(entryName);
    setLastModifier(get_current_unix_timestamp());
    fileCommentLength = 0;
    crc = 0;
    m_originallyInArchive = false;
    m_stored_fullname = entryName;
}

ZipArchiveEntry::~ZipArchiveEntry() {
    if (!fields.empty()) {
        for (auto field = fields.begin(); field != fields.end(); field++) {
            delete (*field);
        }
        fields.clear();
    }
    if (fileName != nullptr) {
        delete[] fileName;
        fileName = nullptr;
    }
    if (fileComment != nullptr) {
        delete[] fileComment;
        fileComment = nullptr;
    }

    if (m_outstandingWriteStream != nullptr) {
        m_outstandingWriteStream->close();
        delete m_outstandingWriteStream;
        m_outstandingWriteStream = nullptr;
    }
    if (compressedBytes != nullptr) {
        compressedBytes->close();
//        delete compressedBytes;
        compressedBytes.reset();
        compressedBytes = nullptr;
    }
    if (uncompressedData != nullptr) {
        uncompressedData->close();
//        delete uncompressedData;
        uncompressedData.reset();
        uncompressedData = nullptr;
    }
    if (cdExtraFields != nullptr) {
        delete[] cdExtraFields;
        cdExtraFields = nullptr;
    }
    if (lfExtraFields != nullptr) {
        delete[] lfExtraFields;
        lfExtraFields = nullptr;
    }
}

bool ZipArchiveEntry::getIsEncrypted() const { return flags & GeneralPurposeBitFlag_IsEncrypted; }
void ZipArchiveEntry::setIsEncrypted(const bool &value) {
    flags = flags & ~GeneralPurposeBitFlag_IsEncrypted | value;
    if (value) {
        setHasDataDescriptor(true);
    }
}
CompressionLevel ZipArchiveEntry::getCompressionLevel() const { return CompressionLevel((flags & 0x6) >> 1); }
void ZipArchiveEntry::setCompressionLevel(CompressionLevel level) { flags = flags & 0xf9 | (level << 1); }
bool ZipArchiveEntry::getHasDataDescriptor() const { return flags & GeneralPurposeBitFlag_DataDescriptor; }
void ZipArchiveEntry::setHasDataDescriptor(bool value) {
    flags = flags & ~GeneralPurposeBitFlag_DataDescriptor | (value << 3);
}
std::string ZipArchiveEntry::getComment() const { return fileComment == nullptr ? "" : std::string(fileComment); }
void ZipArchiveEntry::setComment(const std::string &comment) {
    fileCommentLength = comment.length();
    if (fileComment != nullptr) {
        delete fileComment;
        fileComment = nullptr;
    }
    if (fileCommentLength > 0) {
        fileComment = new char[fileCommentLength];
        memcpy(fileComment, comment.c_str(), comment.length());
    }
}

void ZipArchiveEntry::setCompressionMethod(CompressionMethod value) { compressionMethod = value; }

double ZipArchiveEntry::getLastModifier() const { return dostime_to_unix_timestamp(lastModifier); }
void ZipArchiveEntry::setLastModifier(double value) { lastModifier = unix_timestamp_to_dostime(value); }
uint ZipArchiveEntry::getCryptCRC() const { return getHasDataDescriptor() ? lastModifier << 16 : crc << 16; }

std::string ZipArchiveEntry::getFullName() {
    if (m_stored_fullname.length() == 0) {
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            InfoZIPUnicodeCommentExtraField field;
            if (field.tryRead(*it, &field)) {
                isUnicodeFileName = true;
                m_stored_fullname = field.data;
                return m_stored_fullname;
            }
        }
        if (GeneralPurposeBitFlag_UnicodeFileNameAndComment & flags) {
            isUnicodeFileName = true;
        }
        m_stored_fullname = std::string(fileName);
    }
    return m_stored_fullname;
}

long ZipArchiveEntry::getOffsetOfCompressedData() {
    if (stored_offsetOfCompressedData == -1) {
        IStream *stream = m_archive->getArchiveStream().get();
        stream->seek(headerOffset, SeekOrigin::Begin);
        if (!ZipLocalFileHeader::trySkip(stream))
            throw std::ios::failure("a local file header is corrupt.");

        stored_offsetOfCompressedData = stream->getPosition();
    }
    return stored_offsetOfCompressedData;
}


std::shared_ptr<IStream> ZipArchiveEntry::open() {
    switch (m_archive->getMode()) {
    case ZipArchiveMode_Read:
        return openInReadMode();
    case ZipArchiveMode_Create:
        return openInCreateMode();
    case ZipArchiveMode_Update:
        return openInUpdateMode();
    }
}

void ZipArchiveEntry::Delete() {
    if (m_archive->isClosed())
        return;
    if (m_currentlyOpenForWrite)
        throw std::ios::failure("cannot delete an entry currently open for writing.");
    if (m_archive->getMode() != ZipArchiveMode_Update)
        throw std::ios::failure("delete can only be used when the archive is in update mode.");
    m_archive->removeEntry(this);
    m_archive = nullptr;
}


void ZipArchiveEntry::setFullName(const std::string &entryName) {
    m_stored_fullname = entryName;
    fileNameLength = m_stored_fullname.length();
    fileName = new char[fileNameLength];
    memcpy(fileName, m_stored_fullname.c_str(), fileNameLength);
}

std::shared_ptr<IStream> ZipArchiveEntry::openInReadMode() {
//    IStream *stream =
//        new SubReadStream(m_archive->getArchiveStream(), getOffsetOfCompressedData(), compressedSize, true);
    std::shared_ptr<IStream> stream = std::make_shared<SubReadStream>(
         m_archive->getArchiveStream(), getOffsetOfCompressedData(), compressedSize, true);
    if (getIsEncrypted()) {
        stream = std::make_shared<ZipCryptoStream>(stream, CryptoMode_Decode, this, false);
//        stream = new ZipCryptoStream(stream, CryptoMode_Decode, this, false);
//         stream = new ZipCryptoStream(stream, CryptoMode_Decode, m_archive->getPassword(), false, crc);
    }
    return getDataDecompressor(stream);
}

std::shared_ptr<IStream> ZipArchiveEntry::openInUpdateMode() {
    if (m_currentlyOpenForWrite)
        throw std::ios::failure("entries cannot be opened multiple times in update mode.");
    m_everOpenedForWrite = true;
    m_currentlyOpenForWrite = true;
    auto stream = getUncompressedData();
    stream->seek(0, SeekOrigin::Begin);
    return std::make_shared<WrappedStream>(stream, this, true, [this]() { this->m_currentlyOpenForWrite = false; });
//    return new WrappedStream(stream, this, true, [this]() { this->m_currentlyOpenForWrite = false; });
}

std::shared_ptr<IStream> ZipArchiveEntry::getUncompressedData() {
    if (uncompressedData == nullptr) {
//        uncompressedData = new MemoryStream(uncompressedSize);
        uncompressedData = std::make_shared<MemoryStream>(uncompressedSize);
        if (m_originallyInArchive) {
            try {
                std::shared_ptr<IStream> stream = openInReadMode();
                stream->copyTo(uncompressedData.get(), 8192);
                stream->close();
            } catch (const std::exception &e) {
                uncompressedData->close();
                uncompressedData = nullptr;
                m_currentlyOpenForWrite = false;
                m_everOpenedForWrite = false;
                throw e;
            }
        }
    }
    return uncompressedData;
}


std::shared_ptr<IStream> ZipArchiveEntry::getDataDecompressor(std::shared_ptr<IStream> stream) {
//    IStream *decompressor = stream;
    std::shared_ptr<IStream> decompressor = stream;
    if (compressionMethod == CompressionMethod::Deflate || compressionMethod == CompressionMethod::Deflate64) {
//        decompressor = new DeflateStream(std::shared_ptr<IStream>(stream), DeflateMode_Decompress, -15,
//                                         m_compression_level, false, 8192, uncompressedSize);
        decompressor = std::make_shared<DeflateStream>(stream, DeflateMode_Decompress, -15, m_compression_level, false,
                                                       8192, uncompressedSize);
    }

    return decompressor;
}

std::shared_ptr<IStream> ZipArchiveEntry::getDataCompressor(std::shared_ptr<IStream> stream, bool leaveOpen) {
    std::shared_ptr<IStream> compressorStream = stream;
    bool isZipCrypto = false;
    bool isBase = true;
    if (getIsEncrypted()) {
        compressorStream = std::make_shared<ZipCryptoStream>(stream, CryptoMode_Encode, this, leaveOpen);
//        cryptoStream = new ZipCryptoStream(stream, CryptoMode_Encode, this, leaveOpen);
//        compressorStream = cryptoStream;
        isZipCrypto = true;
        isBase = false;
    }
    switch (compressionMethod) {
    case CompressionMethod::Stored:
        break;
    case CompressionMethod::Deflate:
    case CompressionMethod::Deflate64:
    default:
        compressorStream = std::make_shared<DeflateStream>(compressorStream, DeflateMode_Compress, -15,
                                                           getZlibCompressionLevel(getCompressionLevel()),
                                                           isBase ? leaveOpen && true : false);
//            new DeflateStream(compressorStream, DeflateMode_Compress, -15,
//                              getZlibCompressionLevel(getCompressionLevel()), isBase ? leaveOpen && true :
        //                              false);
        isBase = false;
        break;
    }

    return std::make_shared<CheckSumAndSizeWriteStream>(
        compressorStream, stream, isBase ? leaveOpen && true : false,
        [this](long initialPosition, long currentPosition, uint checkSum) {
            crc = checkSum;
            uncompressedSize = currentPosition;
            compressedSize = m_archive->getArchiveStream()->getPosition() - initialPosition;
        });
//    CheckSumAndSizeWriteStream *checkSumStream =
//        new CheckSumAndSizeWriteStream(compressorStream, stream, isBase ? leaveOpen && true : false,
//                                       [this](long initialPosition, long currentPosition, uint checkSum) {
//                                           crc = checkSum;
//                                           uncompressedSize = currentPosition;
//                                           compressedSize =
//                                               m_archive->getArchiveStream()->getPosition() - initialPosition;
//                                       });
//
//    return checkSumStream;
}

std::shared_ptr<IStream> ZipArchiveEntry::openInCreateMode() {
    if (m_everOpenedForWrite)
        throw std::ios::failure(
            "entries in create mode may only be written to once, and only one entry may be held open at a time.");
    m_everOpenedForWrite = true;
//    CheckSumAndSizeWriteStream *crcStream =
//        (CheckSumAndSizeWriteStream *)getDataCompressor(m_archive->getArchiveStream(), true);
//    return new DirectToArchiveWriterStream(crcStream, this);
    return std::make_shared<DirectToArchiveWriterStream>(getDataCompressor(m_archive->getArchiveStream(), true), this);
}

CompressionMethod ZipArchiveEntry::getCompressionMethod() const { return CompressionMethod(compressionMethod); }

bool ZipArchiveEntry::writeLocalFileHeader() {
    headerOffset = m_archive->getArchiveStream()->getPosition();
    ZipLocalFileHeader header;
    header.signature = ZIP_LOCALFILEHEADER_SIGNATURE;
    header.version = versionToExtract;
    header.flags = flags;
    header.compression = uncompressedSize == 0 ? CompressionLevel_NoCompression : compressionMethod;
    header.lastModifier = lastModifier;
    header.crc = crc;
    header.compressedSize = compressedSize;
    header.uncompressedSize = uncompressedSize;
    header.fileNameLength = fileNameLength;
    header.extraFieldLength = lfExtraFieldsLength;
    IStream *stream = m_archive->getArchiveStream().get();
    stream->write(&header, 0, sizeof(header));
    stream->write(fileName, 0, fileNameLength);
    if (lfExtraFieldsLength > 0) {
        stream->write(lfExtraFields, 0, lfExtraFieldsLength);
    }
    return true;
}

void ZipArchiveEntry::writeCrcAndSizesInLocalHeader() {
    long finalPosition = m_archive->getArchiveStream()->getPosition();
    IStream *stream = m_archive->getArchiveStream().get();
    stream->seek(headerOffset + ZIP_LOCALFILEHEADER_OFFSET_TO_CRC, SeekOrigin::Begin);
    stream->write(&crc, 0, sizeof(crc));
    stream->write(&compressedSize, 0, sizeof(compressedSize));
    stream->write(&uncompressedSize, 0, sizeof(uncompressedSize));
    stream->seek(finalPosition, SeekOrigin::Begin);
}

void ZipArchiveEntry::writeDataDescriptor() {
    IStream *stream = m_archive->getArchiveStream().get();
    stream->write(&crc, 0, sizeof(crc));
    stream->write(&compressedSize, 0, sizeof(compressedSize));
    stream->write(&uncompressedSize, 0, sizeof(uncompressedSize));
}

void ZipArchiveEntry::writeAndFinishLocalEntry() {
    closeStream();
    writeLocalFileHeaderAndDataIfNeeded();
    if (uncompressedData != nullptr) {
        uncompressedData->close();
//        delete uncompressedData;
        uncompressedData.reset();
        uncompressedData = nullptr;
    }
    if (compressedBytes != nullptr) {
        compressedBytes->close();
//        delete compressedBytes;
        compressedBytes.reset();
        compressedBytes = nullptr;
    }
}

void ZipArchiveEntry::writeLocalFileHeaderAndDataIfNeeded() {
    if (uncompressedData != nullptr) {
        uncompressedSize = uncompressedData->getLength();
        lastModifier = get_current_dostime();
        if (getIsEncrypted()) {
            setIsEncrypted(true);
        }
        IStream *entryWriter =
            new DirectToArchiveWriterStream(getDataCompressor(m_archive->getArchiveStream(), true), this);
        uncompressedData->seek(0, SeekOrigin::Begin);
        uncompressedData->copyTo(entryWriter, 8192);
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
            compressedBytes.reset();
//            delete compressedBytes;
            compressedBytes = nullptr;
        }
    }
}

void ZipArchiveEntry::loadLocalHeaderExtraFieldAndCompressedBytesIfNeeded() {
    if (m_originallyInArchive) {
        // 跳转28字节获取localfileheader中的extra_field长度
        m_archive->getArchiveStream()->seek(headerOffset + 28, SeekOrigin::Begin);
        ushort extraFieldLength = 0;
        m_archive->getArchiveStream()->read(&extraFieldLength, 0, 2);
        m_archive->getArchiveStream()->seek(fileNameLength, SeekOrigin::Current);
        if (lfExtraFields != nullptr) {
            delete[] lfExtraFields;
            lfExtraFields = nullptr;
        }
        lfExtraFields = new byte[extraFieldLength];
        lfExtraFieldsLength = extraFieldLength;
        m_archive->getArchiveStream()->read(lfExtraFields, 0, lfExtraFieldsLength);
    }

    if (!m_everOpenedForWrite && m_originallyInArchive) {
        if (compressedBytes != nullptr) {
            compressedBytes->close();
//            delete compressedBytes;
            compressedBytes.reset();
        }
        compressedBytes = std::make_shared<MemoryStream>(compressedSize) ;
        SubReadStream *subRead =
            new SubReadStream(m_archive->getArchiveStream(), getOffsetOfCompressedData(), compressedSize, true);
        char *buffer = new char[8192];
        long readBytes = 0;
        while ((readBytes = subRead->read(buffer, 0, 8192)) != 0) {
            compressedBytes->write(buffer, 0, readBytes);
        }
        delete[] buffer;
        subRead->close();
        delete subRead;
    }
}


void ZipArchiveEntry::writeCentralDirectoryFileHeader() {
    ZipCentralDirectoryRecord record{0};
    record.signature = ZIP_CentralDirectory_SIGNATURE;
    record.versionMadeBy = versionMadeBy;
    record.versionToExtract = versionToExtract;
    record.flags = flags;
    record.compression = compressionMethod;
    record.lastModifier = lastModifier;
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
    IStream *stream = m_archive->getArchiveStream().get();
    stream->write(&record, 0, sizeof(record));
    stream->write(fileName, 0, fileNameLength);
    if (extraFieldLength > 0) {
        stream->write(cdExtraFields, 0, extraFieldLength);
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
        DEFINE_NAPI_FUNCTION("isEncrypted", nullptr, JSGetIsEncrypted, JSSetIsEncrypted, nullptr),
        DEFINE_NAPI_FUNCTION("compressionLevel", nullptr, JSGetCompressionLevel, JSSetCompressionLevel, nullptr),
        DEFINE_NAPI_FUNCTION("fileComment", nullptr, JSGetFileComment, JSSetFileComment, nullptr),
        DEFINE_NAPI_FUNCTION("fullName", nullptr, JSGetFullName, JSSetFullName, nullptr),
        DEFINE_NAPI_FUNCTION("compressionMethod", nullptr, JSGetCompressionMethod, JSSetCompressionMethod, nullptr),
        DEFINE_NAPI_FUNCTION("lastModifier", nullptr, JSGetLastModifier, JSSetLastModifier, nullptr),
        DEFINE_NAPI_FUNCTION("isOpened", nullptr, JSGetIsOpened, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("crc32", nullptr, JSGetCRC, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("delete", JSDelete, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isDeleted", nullptr, JSGetIsDeleted, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("uncompressedSize", nullptr, JSGetUnCompressedSize, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("compressedSize", nullptr, JSGetCompressedSize, nullptr, nullptr)

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

void ZipArchiveEntry::JSDispose(napi_env env, void *data, void *hint) {
    ZipArchiveEntry *entry = static_cast<ZipArchiveEntry *>(data);
    delete entry;
    entry = nullptr;
}

napi_value ZipArchiveEntry::open(napi_env env) {
    std::shared_ptr<IStream> stream = open();
//    openingStream = stream;
    napi_value result = IStream::JSCreateInterface(env, stream);
//     NAPI_CALL(env, napi_create_reference(env, result, 1, &jsOpeningStream))
    return result;
}

napi_value ZipArchiveEntry::JSOpen(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    try {
        return entry->open(env);
    } catch (const std::exception &e) {
        napi_throw_error(env, "ZipArchiveEntry", (std::string("open failed: ") + e.what()).c_str());
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

    uint ref_count = 0;
    NAPI_CALL(env, napi_reference_unref(env, jsEntry, &ref_count))
    if (ref_count > 0) {
        NAPI_CALL(env, napi_delete_reference(env, jsEntry))
    }
    void *_this = nullptr;
    NAPI_CALL(env, napi_remove_wrap(env, value, &_this))


    jsEntry = nullptr;
}

napi_value ZipArchiveEntry::JSSetIsEncrypted(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    if (entry->m_archive->getMode() == ZipArchiveMode_Read)
        napi_throw_error(env, ClassName.c_str(), "can not set isEncrypted in read mode.");
    if (entry->m_everOpenedForWrite)
        napi_throw_error(env, ClassName.c_str(), "can not set isEncrypted after open.");
    bool isEncrypted = false;
    NAPI_CALL(env, napi_get_value_bool(env, argv[0], &isEncrypted))
    entry->setIsEncrypted(isEncrypted);
    return nullptr;
}

napi_value ZipArchiveEntry::JSGetIsEncrypted(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, entry->getIsEncrypted(), &result))
    return result;
}

napi_value ZipArchiveEntry::JSGetCompressionLevel(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, entry->getCompressionLevel(), &result))
    return result;
}

napi_value ZipArchiveEntry::JSSetCompressionLevel(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    if (entry->m_archive->getMode() == ZipArchiveMode_Read)
        napi_throw_error(env, ClassName.c_str(), "can not set compression level in read mode.");
    if (entry->m_everOpenedForWrite)
        napi_throw_error(env, ClassName.c_str(), "can not set compression level after open.");
    int level = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[0], &level))
    entry->setCompressionLevel(CompressionLevel(level));
    return nullptr;
}

napi_value ZipArchiveEntry::JSSetFileComment(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    if (entry->m_archive->getMode() == ZipArchiveMode_Read)
        napi_throw_error(env, ClassName.c_str(), "can not set file comment in read mode.");
    std::string comment = getString(env, argv[0]);
    entry->setComment(comment);
    return nullptr;
}

napi_value ZipArchiveEntry::JSGetFileComment(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, entry->getComment().c_str(), NAPI_AUTO_LENGTH, &result))
    return result;
}

napi_value ZipArchiveEntry::JSGetFullName(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, entry->getFullName().c_str(), NAPI_AUTO_LENGTH, &result))
    return result;
}
napi_value ZipArchiveEntry::JSSetFullName(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    if (entry->m_archive->getMode() == ZipArchiveMode_Read)
        napi_throw_error(env, ClassName.c_str(), "can not set fullName in read mode.");
    std::string name = getString(env, argv[0]);
    entry->setFullName(name);
    return nullptr;
}
napi_value ZipArchiveEntry::JSGetCompressionMethod(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, entry->getCompressionMethod(), &result))
    return result;
}
napi_value ZipArchiveEntry::JSSetCompressionMethod(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    if (entry->m_archive->getMode() == ZipArchiveMode_Read)
        napi_throw_error(env, ClassName.c_str(), "can not set compression method in read mode.");
    if (entry->m_everOpenedForWrite)
        napi_throw_error(env, ClassName.c_str(), "can not set compression method after open.");
    int method = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[0], &method))
    entry->setCompressionMethod(CompressionMethod(method));
    return nullptr;
}
napi_value ZipArchiveEntry::JSSetLastModifier(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(1)
    double timestamp = 0;
    NAPI_CALL(env, napi_get_date_value(env, argv[0], &timestamp))
    entry->setLastModifier(timestamp);
    return nullptr;
}

napi_value ZipArchiveEntry::JSGetLastModifier(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_date(env, entry->getLastModifier(), &result))
    return result;
}

napi_value ZipArchiveEntry::JSGetIsOpened(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, entry->m_everOpenedForWrite, &result))
    return result;
}

napi_value ZipArchiveEntry::JSGetCRC(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, entry->crc, &result))
    return result;
}

void ZipArchiveEntry::Delete(napi_env env) {
    Delete();
    releaseJSEntry(env);
}

napi_value ZipArchiveEntry::JSDelete(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    try {
        entry->Delete(env);
    } catch (const std::exception &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
    }
    return nullptr;
}

napi_value ZipArchiveEntry::JSGetIsDeleted(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO(0)
    ZipArchiveEntry *entry = getEntry(env, _this);
    bool value = entry == nullptr;
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result))
    return result;
}

napi_value ZipArchiveEntry::JSGetUnCompressedSize(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    long size = 0;
    entry->getArchive()->getMode();
    if (entry->getArchive()->getMode() != ZipArchiveMode_Create && !entry->m_everOpenedForWrite) {
        size = entry->uncompressedSize;
    }
    NAPI_CALL(env, napi_create_int64(env, size, &result));
    return result;
}
napi_value ZipArchiveEntry::JSGetCompressedSize(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_ENTRY_INFO_WITH_ENTRY(0)
    napi_value result = nullptr;
    long size = 0;
    entry->getArchive()->getMode();
    if (entry->getArchive()->getMode() != ZipArchiveMode_Create && !entry->m_everOpenedForWrite) {
        size = entry->compressedSize;
    }
    NAPI_CALL(env, napi_create_int64(env, size, &result));
    return result;
}


#endif // DEFINE_ZipArchiveEntry_NAPI
