// SPDX-License-Identifier: GPL-3.0-only

#include <QEventLoop>
#include <QLabel>
#include <QProgressBar>
#include <QTest>
#include <QTimer>

#include "launch/LaunchTask.h"
#include "launch/TaskStepWrapper.h"
#include "ui/dialogs/ProgressDialog.h"

// Exercise launch steps without an instance, authentication or a game process.
class TestLaunchTask : public LaunchTask {
   public:
    TestLaunchTask() : LaunchTask(nullptr) {}

    void executeTask() override
    {
        state = Running;
        onStepFinished();
    }

   protected:
    void emitSucceeded() override { Task::emitSucceeded(); }
    void emitFailed(QString reason) override { Task::emitFailed(reason); }
};

// CurseForge installation waits for network tasks in a nested event loop.
class BlockingInstallTask : public Task {
    Q_OBJECT

   signals:
    void downloading();

   private:
    void executeTask() override
    {
        QEventLoop loop;
        QTimer::singleShot(0, &loop, [&] {
            setStatus("Downloading mods...");
            setProgress(25, 100);
            emit downloading();
            loop.quit();
        });
        loop.exec();
        emitSucceeded();
    }
};

class TaskStepWrapperTest : public QObject {
    Q_OBJECT

   private slots:
    void progressDialogIsVisibleDuringBlockingInstall()
    {
        TestLaunchTask launch;
        auto install = makeShared<BlockingInstallTask>();
        auto step = makeShared<TaskStepWrapper>(&launch, install);
        launch.appendStep(step);

        ProgressDialog dialog;
        bool visibleDuringDownload = false;
        QString displayedStatus;
        int displayedProgress = -1;
        connect(install.get(), &BlockingInstallTask::downloading, this, [&] {
            visibleDuringDownload = dialog.isVisible();
            displayedStatus = dialog.findChild<QLabel*>("globalStatusLabel")->text();
            displayedProgress = dialog.findChild<QProgressBar*>("globalProgressBar")->value();
        });
        connect(&launch, &LaunchTask::requestProgress, this, [&](Task* task) {
            // Match LaunchController's progress-dialog handoff.
            launch.proceed();
            QCOMPARE(dialog.execWithTask(task), QDialog::Accepted);
        });

        launch.start();

        QVERIFY(launch.wasSuccessful());
        QVERIFY(visibleDuringDownload);
        QCOMPARE(displayedStatus, QString("Downloading mods..."));
        QCOMPARE(displayedProgress, 25);
        QVERIFY(!dialog.isVisible());
    }
};

QTEST_MAIN(TaskStepWrapperTest)

#include "TaskStepWrapper_test.moc"
