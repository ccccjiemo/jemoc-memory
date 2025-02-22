//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "zip/ZipArchive.h"
#include "stream/FileStream.h"
#include "stream/MemoryStream.h"
#include "zip/ZipArchiveEntry.h"
#include "zip/ZipHelper.h"
#include "zip/ZipRecord.h"


ZipArchive::ZipArchive(std::shared_ptr<IStream> stream, const ZipArchiveMode mode, const std::string &password,
                       bool leaveOpen)
    : m_mode(mode), m_leaveOpen(leaveOpen), m_passwd(password) {
    if (stream == nullptr)
        throw std::invalid_argument("argument stream is null.");


    switch (mode) {
    case ZipArchiveMode_Read:
        if (!stream->getCanRead())
            throw std::invalid_argument("cannot use read mode on a non-readable stream.");
        if (!stream->getCanSeek()) {
            m_backingStream = stream;
            stream = std::make_shared<MemoryStream>();
            m_backingStream->copyTo(stream.get(), 8192);
            stream->seek(0, SeekOrigin::Begin);
        }
        break;
    case ZipArchiveMode_Update:
        if (!stream->getCanRead() || !stream->getCanSeek() || !stream->getCanWrite())
            throw std::invalid_argument("update mode requires a stream with read, write, and seek capabilities.");
        break;
    case ZipArchiveMode_Create:
        if (!stream->getCanWrite())
            throw std::invalid_argument("cannot use create mode on a non-writable stream.");
        break;
    default:
        throw std::invalid_argument("argument mode is out of range in ZipArchiveMode.");
    }
    m_stream = stream;
    switch (mode) {
    case ZipArchiveMode_Read:
        readEndOfCentralDirectory();
        break;
    case ZipArchiveMode_Create:
        m_readEntries = true;
        break;
    case ZipArchiveMode_Update:
    default:
        if (m_stream->getLength() == 0) {
            m_readEntries = true;
        } else {
            readEndOfCentralDirectory();
        }
        break;
    }
}

ZipArchive::ZipArchive(const std::string &path, const ZipArchiveMode mode, const std::string &password)
    : m_mode(mode), m_leaveOpen(false), m_passwd(password) {
    int fileMode = FILE_MODE_READ;
    switch (mode) {
    case ZipArchiveMode_Read:
        fileMode = FILE_MODE_READ;
        break;
    case ZipArchiveMode_Update:
        fileMode = FILE_MODE_READ | FILE_MODE_WRITE;
        break;
    case ZipArchiveMode_Create:
        fileMode = FILE_MODE_WRITE | FILE_MODE_APPEND;
        break;
    }
    std::shared_ptr<IStream> stream = std::make_shared<FileStream>(path, FILE_MODE(fileMode), 8192);

    new (this) ZipArchive(stream, mode, password, false);
}

ZipArchive::ZipArchive(const int &fd, const long &offset, const long &length, const std::string &password)
    : m_mode(ZipArchiveMode_Read), m_leaveOpen(false), m_passwd(password) {
    std::shared_ptr<IStream> stream = std::make_shared<FileStream>(fd, offset, length);

    new (this) ZipArchive(stream, ZipArchiveMode_Read, password, false);
}


ZipArchive::~ZipArchive() {}

void ZipArchive::close(napi_env env) {
    close();
    for (auto entry = m_entries.begin(); entry != m_entries.end(); entry++) {
        (*entry)->releaseJSEntry(env);
//         delete (*entry);
    }
    m_entries.clear();
    m_entriesDictionary.clear();
}

void ZipArchive::close() {
    if (m_close)
        return;
    m_close = true;

    switch (m_mode) {
    case ZipArchiveMode_Read:
        break;
    case ZipArchiveMode_Update:
    case ZipArchiveMode_Create:
    default:
        writeFile();
        break;
    }

    if ((m_backingStream != nullptr || !m_leaveOpen) && m_stream != nullptr) {
        m_stream->close();
        m_stream = nullptr;
    }
    if (m_backingStream != nullptr && !m_leaveOpen) {
        m_backingStream->close();
        m_backingStream = nullptr;
    }
    m_archiveComment = "";
}

std::string ZipArchive::getComment() const { return m_archiveComment; }
void ZipArchive::setComment(const std::string &comment) { m_archiveComment = comment; }
ZipArchiveMode ZipArchive::getMode() const { return m_mode; }

void ZipArchive::readEndOfCentralDirectory() {
    m_stream->seek(-ZIP_EOCD_SIZEOFRECORD_WITHOUT_SIGNATURE, SeekOrigin::End);
    if (!ZipHelper::seekBackwardsToSignature(m_stream.get(), ZIP_EOCD_SIGNATURE, USHRT_MAX - sizeof(uint)))
        throw std::ios::failure("end of central directory record could not be found.");

//     long eocdStart = m_stream->getPosition();
    ZipEndOfCentralDirectoryRecord eocd;
    if (!ZipEndOfCentralDirectoryRecord::tryReadRecord(m_stream.get(), &eocd))
        throw std::ios::failure("read end of central directory record failed.");

    m_numberOfThisDisk = eocd.diskNumber;
    m_centralDirectoryStart = eocd.directoryOffset;
    if (eocd.startDiskNumber != eocd.diskNumber)
        throw std::ios::failure("split or spanned archives are not supported.");
    m_entriesOnDisk = eocd.entriesOnDisk;
    if (eocd.commentLength > 0) {
        char *comment_buffer = (char *)malloc(eocd.commentLength);
        m_stream->read(comment_buffer, 0, eocd.commentLength);
        m_archiveComment = std::string(comment_buffer);
        free(comment_buffer);
    }
}
void ZipArchive::ensureCentralDirectoryRead() {
    if (!m_readEntries) {
        readCentralDirectory();
        m_readEntries = true;
    }
}

void ZipArchive::readCentralDirectory() {
    m_stream->seek(m_centralDirectoryStart, SeekOrigin::Begin);
    long numberOfEntries = 0;
    ZipCentralDirectoryRecord header;
    while (ZipCentralDirectoryRecord::tryReadRecord(m_stream.get(), false, &header)) {
        addEntry(new ZipArchiveEntry(this, header));
        numberOfEntries++;
    }
    if (numberOfEntries != m_entriesOnDisk)
        throw std::ios::failure("number of entries expected in end of central directory does not correspond to number "
                                "of entries in Central Directory.");
}

void ZipArchive::addEntry(ZipArchiveEntry *entry) {
    m_entries.push_back(entry);
    m_entriesDictionary.emplace(entry->getFullName(), entry);
}


ZipArchiveEntry *ZipArchive::getEntry(const std::string &entryName) {
    if (m_mode == ZipArchiveMode::ZipArchiveMode_Create)
        throw std::ios::failure("cannot access entries in create mode.");

    ensureCentralDirectoryRead();
    auto it = m_entriesDictionary.find(entryName);
    if (it != m_entriesDictionary.end())
        return it->second;
    return nullptr;
}

std::vector<ZipArchiveEntry *> ZipArchive::getEntries() {
    ensureCentralDirectoryRead();
    return m_entries;
}

void ZipArchive::writeFile() {
    if (m_mode == ZipArchiveMode_Update) {
        for (auto entry = m_entries.begin(); entry != m_entries.end(); entry++) {
            (*entry)->loadLocalHeaderExtraFieldAndCompressedBytesIfNeeded();
        }
    }

    m_stream->seek(0, SeekOrigin::Begin);
    m_stream->setLength(0);

    for (auto entry = m_entries.begin(); entry != m_entries.end(); entry++) {
        (*entry)->writeAndFinishLocalEntry();
    }
    long startOfCentralDirectory = m_stream->getPosition();

    for (auto entry = m_entries.begin(); entry != m_entries.end(); entry++) {
        (*entry)->writeCentralDirectoryFileHeader();
    }
    long sizeOfCentralDirectory = m_stream->getPosition() - startOfCentralDirectory;
    writeArchiveEpilogue(startOfCentralDirectory, sizeOfCentralDirectory);
}

void ZipArchive::writeArchiveEpilogue(long startOfCentralDirectory, long sizeOfCentralDirectory) {
    ZipEndOfCentralDirectoryRecord record{};
    record.signature = ZIP_EOCD_SIGNATURE;
    record.diskNumber = 0;
    record.startDiskNumber = 0;
    record.entriesOnDisk = m_entries.size();
    record.entriesInDirectory = m_entries.size();
    record.directorySize = sizeOfCentralDirectory;
    record.directoryOffset = startOfCentralDirectory;
    record.commentLength = m_archiveComment.length();
    m_stream->write(&record, 0, sizeof(record));
    if (m_archiveComment.length() > 0) {
        m_stream->write((char *)m_archiveComment.c_str(), 0, m_archiveComment.length());
    }
}


#ifndef ZIPARCHIVE_NAPI_FUNCTION
#define ZIPARCHIVE_NAPI_FUNCTION

#define GET_ZIPARCHIVE_INFO(number)                                                                                    \
    napi_value argv[number];                                                                                           \
    size_t argc = number;                                                                                              \
    napi_value _this = nullptr;                                                                                        \
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &_this, nullptr))                                          \
    ZipArchive *archive = getZipArchive(env, _this);


std::string ZipArchive::ClassName = "ZipArchive";
napi_ref ZipArchive::cons = nullptr;

ZipArchive *ZipArchive::getZipArchive(napi_env env, napi_value value) {
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, value, &type));
    if (type == napi_undefined)
        napi_throw_error(env, "ZipArchive", "archive is null");

    void *result = nullptr;
    NAPI_CALL(env, napi_unwrap(env, value, &result))
    if (result == nullptr)
        napi_throw_error(env, "ZipArchive", "archive is null");
    return static_cast<ZipArchive *>(result);
}

/**
 * ZipArchiveOption: {mode?: ZipArchiveMode, leaveOpen?: bool, password?: string)
 * constructor(stream: IStream, option?: ZipArchiveOption)
 * constructor(path: string, option?: ZipArchiveOption)
 */
napi_value ZipArchive::JSConstructor(napi_env env, napi_callback_info info) {
    GET_JS_INFO_WITHOUT_STREAM(2)
    napi_valuetype type;
    ZipArchive *zip = nullptr;
    bool leaveOpen = false;
    int mode = ZipArchiveMode_Read;
    std::string passwd;

    NAPI_CALL(env, napi_typeof(env, argv[1], &type))

    if (type == napi_string) {
        passwd = getString(env, argv[1]);

    } else if (type == napi_object) {
        napi_value value = nullptr;

        // 获取leaveOpen
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "leaveOpen", &value))
        NAPI_CALL(env, napi_typeof(env, value, &type));
        if (type == napi_boolean) {
            NAPI_CALL(env, napi_get_value_bool(env, value, &leaveOpen));
        }

        // 获取ZipArchiveMode
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "mode", &value))
        NAPI_CALL(env, napi_typeof(env, value, &type))
        if (type == napi_number) {
            NAPI_CALL(env, napi_get_value_int32(env, value, &mode));
        }

        // 获取password
        NAPI_CALL(env, napi_get_named_property(env, argv[1], "password", &value))
        NAPI_CALL(env, napi_typeof(env, value, &type))
        if (type == napi_string) {
            passwd = getString(env, value);
        }
    }

    // 根据第一参数决定构造函数
    NAPI_CALL(env, napi_typeof(env, argv[0], &type));
    try {
        if (napi_string == type) {
            std::string path = getString(env, argv[0]);
            zip = new ZipArchive(path, ZipArchiveMode(mode), passwd);
        } else {
            std::shared_ptr<IStream> stream = IStream::GetStream(env, argv[0]);
            if (stream == nullptr) {
                napi_value js_fd = nullptr;
                napi_value js_offset = nullptr;
                napi_value js_length = nullptr;
                NAPI_CALL(env, napi_get_named_property(env, argv[0], "fd", &js_fd))
                NAPI_CALL(env, napi_get_named_property(env, argv[0], "offset", &js_offset))
                NAPI_CALL(env, napi_get_named_property(env, argv[0], "length", &js_length))
                int fd = 0;
                long offset = 0;
                long length = 0;
                NAPI_CALL(env, napi_get_value_int32(env, js_fd, &fd))
                NAPI_CALL(env, napi_get_value_int64(env, js_offset, &offset))
                NAPI_CALL(env, napi_get_value_int64(env, js_length, &length))
                zip = new ZipArchive(fd, offset, length, passwd);
            } else {
                zip = new ZipArchive(stream, ZipArchiveMode(mode), passwd, leaveOpen);
            }
//                 napi_throw_error(env, ClassName.c_str(), "invalid argument stream, stream is null");
        }
    } catch (const std::ios::failure &e) {
        napi_throw_error(env, ClassName.c_str(), e.what());
        return nullptr;
    }

    if (zip == nullptr) {
        napi_throw_error(env, ClassName.c_str(), "create ziparchive failed.");
        return nullptr;
    }

    NAPI_CALL(env, napi_wrap(env, _this, zip, JSDispose, nullptr, nullptr));

    return _this;
}

void ZipArchive::Export(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DEFINE_NAPI_FUNCTION("comment", nullptr, JSGetComment, JSSetComment, nullptr),
        DEFINE_NAPI_FUNCTION("getEntry", JSGetEntry, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("entries", nullptr, JSGetEntries, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("mode", nullptr, JSGetMode, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("createEntry", JSCreateEntry, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("close", JSClose, nullptr, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("isClosed", nullptr, JSGetIsClosed, nullptr, nullptr),
        DEFINE_NAPI_FUNCTION("entryNames", nullptr, JSGetEntryNames, nullptr, nullptr),
    };
    napi_value napi_cons = nullptr;
    NAPI_CALL(env, napi_define_class(env, ClassName.c_str(), NAPI_AUTO_LENGTH, JSConstructor, nullptr,
                                     sizeof(desc) / sizeof(desc[0]), desc, &napi_cons))
    NAPI_CALL(env, napi_set_named_property(env, exports, ClassName.c_str(), napi_cons))
    NAPI_CALL(env, napi_create_reference(env, napi_cons, 1, &cons))
}

napi_value ZipArchive::JSGetComment(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, archive->getComment().c_str(), NAPI_AUTO_LENGTH, &result))
    return result;
}

napi_value ZipArchive::JSSetComment(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(1);
    if (argc < 1)
        napi_throw_error(env, "ZipArchive", "set comment invalid argument");
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type))
    if (type != napi_string)
        napi_throw_error(env, "ZipArchive", "set comment invalid argument");

    std::string comment = getString(env, argv[0]);
    archive->setComment(comment);
    return nullptr;
}

napi_value ZipArchive::getEntries(napi_env env) {
    napi_value arr = nullptr;
    std::vector<ZipArchiveEntry *> list = getEntries();
    napi_value jsValue = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, list.size(), &arr))
    for (int i = 0; i < list.size(); i++) {
        jsValue = list[i]->getJSEntry(env);
        NAPI_CALL(env, napi_set_element(env, arr, i, jsValue))
    }
    return arr;
}

napi_value ZipArchive::JSGetEntries(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0);
    return archive->getEntries(env);
}

napi_value ZipArchive::JSGetMode(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0)
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, archive->getMode(), &result))
    return result;
}

napi_value ZipArchive::getEntry(napi_env env, const std::string &entryName) {
    ZipArchiveEntry *entry = getEntry(entryName);

    if (entry == nullptr)
        return nullptr;

    auto it = m_entriesDictionary.find(entryName);
    if (it == m_entriesDictionary.end())
        return nullptr;

    return it->second->getJSEntry(env);
}


napi_value ZipArchive::JSGetEntry(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(1)
    std::string entryName = getString(env, argv[0]);
    return archive->getEntry(env, entryName);
}

ZipArchiveEntry *ZipArchive::createEntry(const std::string &entryName, int compressionLevel) {
    try {
        ZipArchiveEntry *entry = new ZipArchiveEntry(this, entryName, compressionLevel);
        addEntry(entry);
        return entry;
    } catch (const std::exception &e) {
        return nullptr;
    }
}

napi_value ZipArchive::createEntry(napi_env env, const std::string &entryName, int compressionLevel) {
    ZipArchiveEntry *entry = createEntry(entryName, compressionLevel);
    if (entry == nullptr)
        return nullptr;
    return entry->getJSEntry(env);
}

napi_value ZipArchive::JSCreateEntry(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(2)
    std::string entryName = getString(env, argv[0]);
    int level = 0;
    napi_valuetype type;
    NAPI_CALL(env, napi_typeof(env, argv[1], &type))
    if (type == napi_number) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[1], &level))
    }
    return archive->createEntry(env, entryName, level);
}

napi_value ZipArchive::JSClose(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0)
    archive->close(env);
    void *result = nullptr;
    NAPI_CALL(env, napi_remove_wrap(env, _this, &result))
    return nullptr;
}

void ZipArchive::JSDispose(napi_env env, void *data, void *hint) {
    ZipArchive *archive = static_cast<ZipArchive *>(data);
    archive->close(env);
    delete archive;
}


void ZipArchive::removeEntry(ZipArchiveEntry *entry) {
    m_entries.erase(std::remove(m_entries.begin(), m_entries.end(), entry), m_entries.end());
    m_entriesDictionary.erase(entry->getFullName());
}

napi_value ZipArchive::JSGetIsClosed(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0)
    bool value = archive == nullptr || archive->isClosed();
    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, value, &result))
    return result;
}

napi_value ZipArchive::JSGetEntryNames(napi_env env, napi_callback_info info) {
    GET_ZIPARCHIVE_INFO(0)
    auto entries = archive->getEntries();
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, entries.size(), &result))
    napi_value jsName;
    for (int i = 0; i < entries.size(); i++) {
        auto name = entries[i]->getFullName();
        NAPI_CALL(env, napi_create_string_utf8(env, name.c_str(), name.size(), &jsName));
        NAPI_CALL(env, napi_set_element(env, result, i, jsName));
    }
    return result;
}

#endif // ZIPARCHIVE_NAPI_FUNCTION