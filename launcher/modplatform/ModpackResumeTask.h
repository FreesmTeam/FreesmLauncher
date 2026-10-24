// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QPointer>
#include <QWidget>

#include "launch/LaunchStep.h"

class MinecraftInstance;

class ModpackResumeTask : public LaunchStep {
    Q_OBJECT

   public:
    ModpackResumeTask(LaunchTask* parent, MinecraftInstance* instance, bool explicitlyOffline, QWidget* parentWidget = nullptr);

    static bool hasPendingManifest(const QString& instanceRoot);
    bool canAbort() const override;
    bool abort() override;

   protected:
    void executeTask() override;

   private:
    void reportProgress();
    void installFinished();

    MinecraftInstance* m_instance;
    bool m_explicitlyOffline;
    QPointer<QWidget> m_parentWidget;
    Task::Ptr m_task;
    QString m_pendingPath;
    bool m_aborted = false;
};
