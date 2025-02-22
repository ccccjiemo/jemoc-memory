//
// Created on 2025/1/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef JEMOC_STREAM_TEST_ZIPARCHIVE_H
#define JEMOC_STREAM_TEST_ZIPARCHIVE_H
#include "IStream.h"
#include <napi/native_api.h>
#include <string>
#include <unordered_map>
#include <vector>

class ZipArchiveEntry;

enum ZipArchiveMode { ZipArchiveMode_Read, ZipArchiveMode_Update, ZipArchiveMode_Create };

class ZipArchive {
public:
    ZipArchive(std::shared_ptr<IStream> stream, const ZipArchiveMode mode, const std::string &password, bool leaveOpen);
    ZipArchive(const std::string &path, const ZipArchiveMode mode, const std::string &password);
    ZipArchive(const int &fd, const long &offset, const long &length, const std::string &password);
    ~ZipArchive();
    std::string getComment() const;
    void setComment(const std::string &comment);
    ZipArchiveMode getMode() const;
    ZipArchiveEntry *createEntry(const std::string &entryName, int compressionLevel);
    ZipArchiveEntry *getEntry(const std::string &entryName);
    std::vector<ZipArchiveEntry *> getEntries();
    std::shared_ptr<IStream> &getArchiveStream() { return m_stream; }
    std::string getPassword() const { return m_passwd; }
    void close();
    bool isClosed() const { return m_close; }


public:
    napi_value getEntries(napi_env env);
    napi_value getEntry(napi_env env, const std::string &entryName);
    napi_value createEntry(napi_env, const std::string &entryName, int compressionLevel);
    void close(napi_env env);
    void removeEntry(ZipArchiveEntry *entry);

public:
    static napi_value JSConstructor(napi_env env, napi_callback_info info);
    static void JSDispose(napi_env env, void *data, void *hint);
    static void Export(napi_env env, napi_value exports);
    static napi_value JSGetComment(napi_env env, napi_callback_info info);
    static napi_value JSSetComment(napi_env env, napi_callback_info info);
    static napi_value JSGetEntries(napi_env env, napi_callback_info info);
    static napi_value JSGetMode(napi_env env, napi_callback_info info);
    static napi_value JSGetEntry(napi_env env, napi_callback_info info);
    static napi_value JSCreateEntry(napi_env env, napi_callback_info info);
    static napi_value JSClose(napi_env env, napi_callback_info info);
    static std::string ClassName;
    static napi_ref cons;
    static ZipArchive *getZipArchive(napi_env env, napi_value value);
    static napi_value JSGetIsClosed(napi_env env, napi_callback_info info);
    static napi_value JSGetEntryNames(napi_env env, napi_callback_info info);

private:
    void readEndOfCentralDirectory();
    void ensureCentralDirectoryRead();
    void readCentralDirectory();
    void addEntry(ZipArchiveEntry *entry);
    void writeFile();
    void writeArchiveEpilogue(long startOfCentralDirectory, long sizeOfCentralDirectory);

private:
    std::shared_ptr<IStream> m_stream = nullptr;
    const ZipArchiveMode m_mode;
    const std::string m_passwd;
    const bool m_leaveOpen;
    std::shared_ptr<IStream> m_backingStream = nullptr;
    std::vector<ZipArchiveEntry *> m_entries;
    std::unordered_map<std::string, ZipArchiveEntry *> m_entriesDictionary;
    bool m_readEntries = false;
    long m_centralDirectoryStart = 0;
    uint32_t m_numberOfThisDisk = 0;
    uint32_t m_entriesOnDisk = 0;
    std::string m_archiveComment;
    bool m_close = false;
};

#endif // JEMOC_STREAM_TEST_ZIPARCHIVE_H
