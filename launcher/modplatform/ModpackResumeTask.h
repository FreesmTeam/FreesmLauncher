// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>
#include <QWidget>

#include "tasks/Task.h"

class MinecraftInstance;

class ModpackResumeTask : public Task {
    Q_OBJECT

   public:
    ModpackResumeTask(MinecraftInstance* instance, bool explicitlyOffline, QWidget* parent = nullptr);

    static bool hasPendingManifest(const QString& instanceRoot);
    bool canAbort() const override;
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void installFinished();

    MinecraftInstance* m_instance;
    bool m_explicitlyOffline;
    QPointer<QWidget> m_parent;
    Task::Ptr m_task;
    QString m_pendingPath;
    bool m_aborted = false;
};
