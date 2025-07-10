#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <QByteArray>
#include <QDateTime>
#include <QStringList>
#include <memory>

namespace MMCZip {
class ArchiveReader {
   public:
    using ArchivePtr = std::unique_ptr<struct archive, int (*)(struct archive*)>;
    ArchiveReader(QString fileName) : m_archivePath(fileName) {}
    virtual ~ArchiveReader() = default;

    QStringList getFiles();
    QString getZipName();
    bool collectFiles(bool onlyFiles = true);
    bool exists(const QString& filePath) const;

    class File {
       public:
        File() : m_archive(ArchivePtr(archive_read_new(), archive_read_free)) {}
        virtual ~File() {}

        QString filename();
        bool isFile();
        QDateTime dateTime();
        const char* error();

        QByteArray readAll(int* outStatus = nullptr);
        bool skip();
        bool writeFile(archive* out, QString targetFileName = "", bool notBlock = false);

       private:
        int readNextHeader();

       private:
        friend ArchiveReader;
        ArchivePtr m_archive;
        archive_entry* m_entry;
    };

    std::unique_ptr<File> goToFile(QString filename);
    bool parse(std::function<bool(File*)>);
    bool parse(std::function<bool(File*, bool&)>);

   private:
    QString m_archivePath;

    QStringList m_fileNames = {};
};
}  // namespace MMCZip