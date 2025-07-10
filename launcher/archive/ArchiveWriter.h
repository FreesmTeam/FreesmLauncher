#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <QByteArray>
#include <QFileDevice>
#include "archive/ArchiveReader.h"

namespace MMCZip {

class ArchiveWriter {
   public:
    ArchiveWriter(const QString& archiveName);
    virtual ~ArchiveWriter();

    bool open();
    bool close();

    bool addFile(const QString& fileName, const QString& fileDest);
    bool addFile(const QString& fileDest, const QByteArray& data);
    bool addFile(ArchiveReader::File* f);

   private:
    struct archive* m_archive = nullptr;
    QString m_filename;
};
}  // namespace MMCZip