// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QJsonObject>
#include <QQueue>
#include <QUrl>
#include <memory>
#include <vector>

#include "modplatform/helpers/ModpackCache.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;
class Resource;
class QWidget;

class ModrinthPackInstallTask final : public Task {
    Q_OBJECT

   public:
    ModrinthPackInstallTask(MinecraftInstance* instance, QJsonObject manifest, bool trustedSource, QWidget* parent = nullptr);
    ~ModrinthPackInstallTask() override;

    bool abort() override;

   protected:
    void executeTask() override;

   private:
    friend class ModrinthCreationTask;
    struct File {
        QString key;
        QString enabledPath;
        QString disabledPath;
        QByteArray hash;
        MetaEntryPtr cacheEntry;
        QQueue<QUrl> downloads;
        bool optional = false;
        bool enabled = true;
        bool unsupported = false;
        bool completed = false;

        QString path() const { return enabled ? enabledPath : disabledPath; }
        QString alternatePath() const { return enabled ? disabledPath : enabledPath; }
    };

    bool prepare();
    void installFiles();
    bool validatePath(const QString& path) const;
    bool completeFile(File& file);
    void downloadNextRound();
    void finishDownloads();
    void finish();

    MinecraftInstance* m_instance;
    QJsonObject m_manifest;
    bool m_trustedSource;
    QWidget* m_parent;
    bool m_isUpdate = false;
    bool m_aborted = false;
    QString m_error;
    std::vector<File> m_files;
    NetJob::Ptr m_downloadJob;
    Task::Ptr m_metadataTask;
    std::vector<std::unique_ptr<Resource>> m_resources;
};
