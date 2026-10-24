// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>

#include "modplatform/flame/PackManifest.h"
#include "modplatform/helpers/PendingModpackManifest.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;
class QDialog;
class QWidget;

class FlamePackInstallTask final : public Task {
    Q_OBJECT

   public:
    FlamePackInstallTask(MinecraftInstance* instance,
                         QJsonObject manifest,
                         Modpack::InstallMode mode,
                         bool trustedSource,
                         QWidget* parent = nullptr);

    bool canAbort() const override { return true; }
    bool abort() override;
    QJsonObject remainingManifest() const { return m_pending.remaining(); }
    bool canPreserve() const;

   signals:
    void readyForDownload();

   protected:
    void executeTask() override;

   private:
    void installFiles(const Flame::Manifest& manifest);
    bool runTask(const Task::Ptr& task);
    bool checkPath(const QString& path);
    bool validFile(const Flame::File& file, const QString& path) const;
    QString reusableFile(const Flame::File& file, bool disabled);
    bool completeFile(const Flame::File& file, QString path, bool disabled);
    bool saveProgress();
    bool installWorld(const QString& path, bool disabled, QString* installedPath);
    void rollbackWorld(const QString& path);
    void fail(const QString& reason, bool unsafe = false);

    MinecraftInstance* m_instance;
    Modpack::PendingManifest m_pending;
    Modpack::InstallMode m_mode;
    bool m_trustedSource;
    QWidget* m_parent;
    bool m_abort = false;
    bool m_canPreserve = false;
    QString m_error;
    QPointer<QDialog> m_dialog;
    NetJob::Ptr m_filesJob;
    QSet<QString> m_completed;
    QList<Flame::File> m_metadataFiles;
};
