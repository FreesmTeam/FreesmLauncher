#pragma once

#include <QDir>
#include <QFuture>
#include <QFutureWatcher>
#include "archive/ArchiveReader.h"
#include "tasks/Task.h"

namespace MMCZip {

class ExtractZipTask : public Task {
    Q_OBJECT
   public:
    ExtractZipTask(QString input, QDir outputDir, QString subdirectory = "")
        : m_input(input), m_output_dir(outputDir), m_subdirectory(subdirectory)
    {}
    virtual ~ExtractZipTask() = default;

    using ZipResult = std::optional<QString>;

   protected:
    virtual void executeTask() override;
    bool abort() override;

    ZipResult extractZip();
    void finish();

   private:
    ArchiveReader m_input;
    QDir m_output_dir;
    QString m_subdirectory;

    QFuture<ZipResult> m_zip_future;
    QFutureWatcher<ZipResult> m_zip_watcher;
};
}  // namespace MMCZip