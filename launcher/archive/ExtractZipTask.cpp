#include "ExtractZipTask.h"
#include <QtConcurrent>
#include "FileSystem.h"
#include "archive/ArchiveReader.h"

namespace MMCZip {

void ExtractZipTask::executeTask()
{
    m_zip_future = QtConcurrent::run(QThreadPool::globalInstance(), [this]() { return extractZip(); });
    connect(&m_zip_watcher, &QFutureWatcher<ZipResult>::finished, this, &ExtractZipTask::finish);
    m_zip_watcher.setFuture(m_zip_future);
}

auto ExtractZipTask::extractZip() -> ZipResult
{
    auto target = m_output_dir.absolutePath();
    auto target_top_dir = QUrl::fromLocalFile(target);

    QStringList extracted;

    qDebug() << "Extracting subdir" << m_subdirectory << "from" << m_input.getZipName() << "to" << target;
    if (!m_input.collectFiles()) {
        return ZipResult(tr("Failed to enumerate files in archive"));
    }
    if (m_input.getFiles().isEmpty()) {
        logWarning(tr("Extracting empty archives seems odd..."));
        return ZipResult();
    }

    int flags;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    std::unique_ptr<archive, void (*)(archive*)> extPtr(archive_write_disk_new(), [](archive* a) {
        archive_write_close(a);
        archive_write_free(a);
    });
    auto ext = extPtr.get();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    setStatus("Extracting files...");
    setProgress(0, m_input.getFiles().count());
    ZipResult result;
    if (!m_input.parse([this, &result, &target, &target_top_dir, ext, &extracted](ArchiveReader::File* f) {
            if (m_zip_future.isCanceled())
                return false;
            setProgress(m_progress + 1, m_progressTotal);
            QString file_name = f->filename();
            if (!file_name.startsWith(m_subdirectory)) {
                f->skip();
                return true;
            }

            auto relative_file_name = QDir::fromNativeSeparators(file_name.mid(m_subdirectory.size()));
            auto original_name = relative_file_name;
            setStatus("Unpacking: " + relative_file_name);

            // Fix subdirs/files ending with a / getting transformed into absolute paths
            if (relative_file_name.startsWith('/'))
                relative_file_name = relative_file_name.mid(1);

            // Fix weird "folders with a single file get squashed" thing
            QString sub_path;
            if (relative_file_name.contains('/') && !relative_file_name.endsWith('/')) {
                sub_path = relative_file_name.section('/', 0, -2) + '/';
                FS::ensureFolderPathExists(FS::PathCombine(target, sub_path));

                relative_file_name = relative_file_name.split('/').last();
            }

            QString target_file_path;
            if (relative_file_name.isEmpty()) {
                target_file_path = target + '/';
            } else {
                target_file_path = FS::PathCombine(target_top_dir.toLocalFile(), sub_path, relative_file_name);
                if (relative_file_name.endsWith('/') && !target_file_path.endsWith('/'))
                    target_file_path += '/';
            }

            if (!target_top_dir.isParentOf(QUrl::fromLocalFile(target_file_path))) {
                result = ZipResult(tr("Extracting %1 was cancelled, because it was effectively outside of the target path %2")
                                       .arg(relative_file_name, target));
                return false;
            }

            if (!f->writeFile(ext, target_file_path)) {
                result = ZipResult(tr("Failed to extract file %1 to %2").arg(original_name, target_file_path));
                return false;
            }
            extracted.append(target_file_path);

            qDebug() << "Extracted file" << relative_file_name << "to" << target_file_path;
            return true;
        })) {
        FS::removeFiles(extracted);
        return result.has_value() || m_zip_future.isCanceled() ? result
                                                               : ZipResult(tr("Failed to parse file %1").arg(m_input.getZipName()));
    }
    return ZipResult();
}

void ExtractZipTask::finish()
{
    if (m_zip_future.isCanceled()) {
        emitAborted();
    } else if (auto result = m_zip_future.result(); result.has_value()) {
        emitFailed(result.value());
    } else {
        emitSucceeded();
    }
}

bool ExtractZipTask::abort()
{
    if (m_zip_future.isRunning()) {
        m_zip_future.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `m_build_zip_future` actually cancels, which may not occur
        // immediately.
        return true;
    }
    return false;
}

}  // namespace MMCZip