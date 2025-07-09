
#include "ExportToZipTask.h"

#include <QtConcurrent>

#include "FileSystem.h"

namespace MMCZip {
void ExportToZipTask::executeTask()
{
    setStatus("Adding files...");
    setProgress(0, m_files.length());
    m_build_zip_future = QtConcurrent::run(QThreadPool::globalInstance(), [this]() { return exportZip(); });
    connect(&m_build_zip_watcher, &QFutureWatcher<ZipResult>::finished, this, &ExportToZipTask::finish);
    m_build_zip_watcher.setFuture(m_build_zip_future);
}

auto ExportToZipTask::exportZip() -> ZipResult
{
    if (!m_dir.exists()) {
        return ZipResult(tr("Folder doesn't exist"));
    }
    if (!m_output.open()) {
        return ZipResult(tr("Could not create file"));
    }

    for (auto fileName : m_extra_files.keys()) {
        if (m_build_zip_future.isCanceled())
            return ZipResult();
        if (!m_output.addFile(fileName, m_extra_files[fileName])) {
            return ZipResult(tr("Could not add:") + fileName);
        }
    }

    for (const QFileInfo& file : m_files) {
        if (m_build_zip_future.isCanceled())
            return ZipResult();

        auto absolute = file.absoluteFilePath();
        auto relative = m_dir.relativeFilePath(absolute);
        setStatus("Compressing: " + relative);
        setProgress(m_progress + 1, m_progressTotal);
        if (m_follow_symlinks) {
            if (file.isSymLink())
                absolute = file.symLinkTarget();
            else
                absolute = file.canonicalFilePath();
        }

        if (!m_exclude_files.contains(relative) && !m_output.addFile(absolute, m_destination_prefix + relative)) {
            return ZipResult(tr("Could not read and compress %1").arg(relative));
        }
    }

    if (!m_output.close()) {
        return ZipResult(tr("A zip error occurred"));
    }
    return ZipResult();
}

void ExportToZipTask::finish()
{
    if (m_build_zip_future.isCanceled()) {
        FS::deletePath(m_output_path);
        emitAborted();
    } else if (auto result = m_build_zip_future.result(); result.has_value()) {
        FS::deletePath(m_output_path);
        emitFailed(result.value());
    } else {
        emitSucceeded();
    }
}

bool ExportToZipTask::abort()
{
    if (m_build_zip_future.isRunning()) {
        m_build_zip_future.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `m_build_zip_future` actually cancels, which may not occur
        // immediately.
        return true;
    }
    return false;
}
}  // namespace MMCZip