
#include "ArchiveWriter.h"
#include <archive.h>

#include <QFile>
#include <QFileInfo>

#include <memory>

namespace MMCZip {

ArchiveWriter::ArchiveWriter(const QString& archiveName) : m_filename(archiveName) {}

ArchiveWriter::~ArchiveWriter()
{
    close();
}

bool ArchiveWriter::open()
{
    if (m_filename.isEmpty()) {
        qCritical() << "Archive m_filename not set.";
        return false;
    }

    m_archive = archive_write_new();
    if (!m_archive) {
        qCritical() << "Archive not initialized.";
        return false;
    }

    QString lowerName = m_filename.toLower();
    if (lowerName.endsWith(".tar.gz") || lowerName.endsWith(".tgz")) {
        archive_write_set_format_pax_restricted(m_archive);
        archive_write_add_filter_gzip(m_archive);
    } else if (lowerName.endsWith(".tar.bz2") || lowerName.endsWith(".tbz")) {
        archive_write_set_format_pax_restricted(m_archive);
        archive_write_add_filter_bzip2(m_archive);
    } else if (lowerName.endsWith(".tar.xz") || lowerName.endsWith(".txz")) {
        archive_write_set_format_pax_restricted(m_archive);
        archive_write_add_filter_xz(m_archive);
    } else if (lowerName.endsWith(".zip") || lowerName.endsWith(".jar")) {
        archive_write_set_format_zip(m_archive);
    } else if (lowerName.endsWith(".tar")) {
        archive_write_set_format_pax_restricted(m_archive);
    } else {
        qCritical() << "Unknown archive format:" << m_filename;
        return false;
    }

    auto archiveNameUtf8 = m_filename.toUtf8();
    if (archive_write_open_filename(m_archive, archiveNameUtf8.constData()) != ARCHIVE_OK) {
        qCritical() << "Failed to open archive file:" << m_filename << "-" << archive_error_string(m_archive);
        return false;
    }

    return true;
}

bool ArchiveWriter::close()
{
    bool success = true;
    if (m_archive) {
        if (archive_write_close(m_archive) != ARCHIVE_OK) {
            qCritical() << "Failed to close archive" << m_filename << "-" << archive_error_string(m_archive);
            success = false;
        }
        if (archive_write_free(m_archive) != ARCHIVE_OK) {
            qCritical() << "Failed to free archive" << m_filename << "-" << archive_error_string(m_archive);
            success = false;
        }
        m_archive = nullptr;
    }
    return success;
}

bool ArchiveWriter::addFile(const QString& fileName, const QString& fileDest)
{
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        qCritical() << "File does not exists:" << fileInfo.filePath();
        return false;
    }

    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        qCritical() << "Failed to create archive entry";
        return false;
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname(entry, fileDestUtf8.constData());
    archive_entry_set_perm(entry, fileInfo.permissions() & 0777);

    if (fileInfo.isSymLink()) {
        auto target = fileInfo.symLinkTarget().toUtf8();
        archive_entry_set_filetype(entry, AE_IFLNK);
        archive_entry_set_symlink(entry, target.constData());
        archive_entry_set_size(entry, 0);

        if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
            qCritical() << "Failed to write symlink header for:" << fileDest << "-" << archive_error_string(m_archive);
            return false;
        }
        return true;
    }

    if (!fileInfo.isFile()) {
        qCritical() << "Unsupported file type:" << fileInfo.filePath();
        return false;
    }

    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Failed to open file: " << fileInfo.filePath();
        return false;
    }

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, file.size());

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        qCritical() << "Failed to write header for: " << fileDest;
        return false;
    }

    constexpr qint64 chunkSize = 8192;
    QByteArray buffer;
    buffer.resize(chunkSize);

    while (!file.atEnd()) {
        auto bytesRead = file.read(buffer.data(), chunkSize);
        if (bytesRead < 0) {
            qCritical() << "Read error in file: " << fileInfo.filePath();
            return false;
        }

        if (archive_write_data(m_archive, buffer.constData(), bytesRead) < 0) {
            qCritical() << "Write error in archive for: " << fileDest;
            return false;
        }
    }
    return true;
}

bool ArchiveWriter::addFile(const QString& fileDest, const QByteArray& data)
{
    std::unique_ptr<archive_entry, void (*)(archive_entry*)> entry_ptr(archive_entry_new(), archive_entry_free);
    auto entry = entry_ptr.get();
    if (!entry) {
        qCritical() << "Failed to create archive entry";
        return false;
    }

    auto fileDestUtf8 = fileDest.toUtf8();
    archive_entry_set_pathname(entry, fileDestUtf8.constData());
    archive_entry_set_perm(entry, 0644);

    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_size(entry, data.size());

    if (archive_write_header(m_archive, entry) != ARCHIVE_OK) {
        qCritical() << "Failed to write header for: " << fileDest;
        return false;
    }

    if (archive_write_data(m_archive, data.constData(), data.size()) < 0) {
        qCritical() << "Write error in archive for: " << fileDest;
        return false;
    }
    return true;
}

bool ArchiveWriter::addFile(ArchiveReader::File* f)
{
    return f->writeFile(m_archive, "", true);
}
}  // namespace MMCZip