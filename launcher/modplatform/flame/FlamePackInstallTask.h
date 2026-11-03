// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>

#include "modplatform/flame/PackManifest.h"
#include "modplatform/helpers/ModpackCache.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;
class QDialog;
class QWidget;

class FlamePackInstallTask final : public Task {
    Q_OBJECT

   public:
    FlamePackInstallTask(MinecraftInstance* instance, QJsonObject manifest, QWidget* parent = nullptr);

    bool canAbort() const override { return true; }
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void installFiles(const Flame::Manifest& manifest);
    bool runTask(const Task::Ptr& task);
    bool checkPath(const QString& path);
    bool validFile(const Flame::File& file, const QString& path) const;
    bool installCachedFile(const Flame::File& file, const MetaEntryPtr& entry, const QString& path, bool disabled);
    bool completeFile(const Flame::File& file, QString path, bool disabled);
    bool installWorld(const QString& path, bool disabled, QString* installedPath);
    void rollbackWorld(const QString& path);
    void fail(const QString& reason);

    MinecraftInstance* m_instance;
    QJsonObject m_manifest;
    QWidget* m_parent;
    bool m_abort = false;
    QString m_error;
    QPointer<QDialog> m_dialog;
    NetJob::Ptr m_filesJob;
    QSet<QString> m_completed;
    QList<Flame::File> m_metadataFiles;
};
