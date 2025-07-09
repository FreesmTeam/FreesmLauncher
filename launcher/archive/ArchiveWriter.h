#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <QByteArray>
#include <QFileDevice>

namespace MMCZip {

class ArchiveWriter {
   public:
    ArchiveWriter(const QString& archiveName);
    ~ArchiveWriter();

    bool open();
    bool close();

    bool addFile(const QString& fileName, const QString& fileDest);
    bool addFile(const QString& fileDest, const QByteArray& data);
    bool addFile(archive* src, archive_entry* entry = nullptr);

   private:
    struct archive* m_archive = nullptr;
    QString m_filename;
};
}  // namespace MMCZip