#pragma once

#include <QDir>
#include <QFileInfoList>
#include <QFuture>
#include <QFutureWatcher>

#include "ArchiveWriter.h"
#include "tasks/Task.h"

namespace MMCZip {
class ExportToZipTask : public Task {
    Q_OBJECT
   public:
    ExportToZipTask(QString outputPath, QDir dir, QFileInfoList files, QString destinationPrefix = "", bool followSymlinks = false)
        : m_output_path(outputPath)
        , m_output(outputPath)
        , m_dir(dir)
        , m_files(files)
        , m_destination_prefix(destinationPrefix)
        , m_follow_symlinks(followSymlinks)
    {
        setAbortable(true);
        // m_output.setUtf8Enabled(utf8Enabled); // ignore for now
        // need to test:
        // - https://github.com/PrismLauncher/PrismLauncher/pull/2225
        // - https://github.com/PrismLauncher/PrismLauncher/pull/2353
    };
    ExportToZipTask(QString outputPath, QString dir, QFileInfoList files, QString destinationPrefix = "", bool followSymlinks = false)
        : ExportToZipTask(outputPath, QDir(dir), files, destinationPrefix, followSymlinks) {};

    virtual ~ExportToZipTask() = default;

    void setExcludeFiles(QStringList excludeFiles) { m_exclude_files = excludeFiles; }
    void addExtraFile(QString fileName, QByteArray data) { m_extra_files.insert(fileName, data); }

    using ZipResult = std::optional<QString>;

   protected:
    virtual void executeTask() override;
    bool abort() override;

    ZipResult exportZip();
    void finish();

   private:
    QString m_output_path;
    ArchiveWriter m_output;
    QDir m_dir;
    QFileInfoList m_files;
    QString m_destination_prefix;
    bool m_follow_symlinks;
    QStringList m_exclude_files;
    QHash<QString, QByteArray> m_extra_files;

    QFuture<ZipResult> m_build_zip_future;
    QFutureWatcher<ZipResult> m_build_zip_watcher;
};
}  // namespace MMCZip