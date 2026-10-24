// SPDX-License-Identifier: GPL-3.0-only

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QLabel>
#include <QProgressBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include "launch/LaunchTask.h"
#include "launch/TaskStepWrapper.h"
#include "modplatform/ModpackResumeTask.h"
#include "modplatform/helpers/PendingModpackManifest.h"
#include "settings/INISettingsObject.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/UntrustedModsDialog.h"

// Exercise launch steps without an instance, authentication or a game process.
class TestLaunchTask : public LaunchTask {
   public:
    TestLaunchTask(MinecraftInstance* instance = nullptr) : LaunchTask(instance) {}

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
    void resumeProgressFollowsTrustConfirmation_data()
    {
        QTest::addColumn<bool>("modrinth");
        QTest::addColumn<bool>("acceptTrust");
        QTest::newRow("Modrinth-reject") << true << false;
        QTest::newRow("CurseForge-reject") << false << false;
        QTest::newRow("Modrinth-accept") << true << true;
        QTest::newRow("CurseForge-accept") << false << true;
    }

    void resumeProgressFollowsTrustConfirmation()
    {
        QFETCH(bool, modrinth);
        QFETCH(bool, acceptTrust);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath("minecraft/mods"));
        QFile mod(temp.filePath("minecraft/mods/untrusted.jar"));
        QVERIFY(mod.open(QIODevice::WriteOnly));
        mod.close();

        INISettingsObject globalSettings(temp.filePath("global.cfg"));
        for (const auto& key :
             { "ShowGameTime", "RecordGameTime", "PreLaunchCommand", "WrapperCommand", "PostExitCommand", "ShowConsole", "AutoCloseConsole",
               "ShowConsoleOnError", "LogPrePostOutput", "ConsoleOverflowStop", "EnableDiscordRichPresence" }) {
            globalSettings.registerSetting(key, false);
        }
        globalSettings.registerSetting("ConsoleMaxLines", 1000);
        MinecraftInstance instance(&globalSettings, std::make_unique<INISettingsObject>(temp.filePath("instance.cfg")), temp.path());

        const auto format = modrinth ? Modpack::Format::Modrinth : Modpack::Format::CurseForge;
        const QJsonObject manifest =
            modrinth ? QJsonObject{ { "formatVersion", 1 },
                                    { "game", "minecraft" },
                                    { "dependencies", QJsonObject{ { "minecraft", "1.20.1" } } },
                                    { "files", QJsonArray{} } }
                     : QJsonObject{ { "manifestType", "minecraftModpack" }, { "manifestVersion", 1 }, { "files", QJsonArray{} } };
        const auto pendingPath = Modpack::pendingManifestPath(temp.path(), format);
        QVERIFY(QDir().mkpath(QFileInfo(pendingPath).absolutePath()));
        Modpack::PendingManifest pending(format, manifest);
        QString error;
        QVERIFY2(pending.save(pendingPath, &error), qPrintable(error));

        TestLaunchTask launch(&instance);
        auto resume = makeShared<ModpackResumeTask>(&launch, &instance, false);
        launch.appendStep(resume);
        int progressRequests = 0;
        connect(&launch, &LaunchTask::requestProgress, this, [&](Task* task) {
            ++progressRequests;
            ProgressDialog dialog;
            launch.proceed();
            // Cancel at the handoff: queued content work must settle without
            // starting real network requests or trapping the modal dialog.
            QVERIFY(task->abort());
            dialog.execWithTask(task);
        });

        bool confirmedDialogShown = false;
        bool progressVisibleDuringConfirmation = false;
        QTimer observeDialog;
        connect(&observeDialog, &QTimer::timeout, this, [&] {
            for (auto* widget : QApplication::topLevelWidgets()) {
                auto* confirmation = qobject_cast<UntrustedModsDialog*>(widget);
                if (!confirmation || !confirmation->isVisible())
                    continue;
                confirmedDialogShown = true;
                for (auto* other : QApplication::topLevelWidgets()) {
                    if (qobject_cast<ProgressDialog*>(other) && other->isVisible())
                        progressVisibleDuringConfirmation = true;
                }
                if (acceptTrust)
                    confirmation->accept();
                else
                    confirmation->reject();
            }
        });
        observeDialog.start();
        launch.start();
        QTRY_VERIFY_WITH_TIMEOUT(launch.isFinished(), 5000);

        QVERIFY(confirmedDialogShown);
        QVERIFY(!progressVisibleDuringConfirmation);
        QCOMPARE(progressRequests, acceptTrust ? 1 : 0);
        QCOMPARE(resume->getState(), Task::State::AbortedByUser);
        const auto remaining = Modpack::PendingManifest::load(format, pendingPath, &error);
        QVERIFY2(remaining.has_value(), qPrintable(error));
        QCOMPARE(remaining->remaining(), manifest);
    }

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
