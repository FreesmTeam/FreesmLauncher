// SPDX-License-Identifier: GPL-3.0-only
#include <QFileInfo>
#include <QNetworkReply>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "FileSystem.h"
#include "InstanceCreationTask.h"
#include "InstanceList.h"
#include "modplatform/helpers/ModpackCache.h"
#include "net/ChecksumValidator.h"
#include "net/FileSink.h"
#include "net/MetaCacheSink.h"

class SuccessfulReply final : public QNetworkReply {
   public:
    SuccessfulReply()
    {
        setUrl(QUrl("http://127.0.0.1/file"));
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        open(QIODevice::ReadOnly);
        setFinished(true);
    }
    void abort() override {}

   protected:
    qint64 readData(char*, qint64) override { return -1; }
};

// Exercise the real creation/staging failure contract without a platform API or Application singleton.
class InterruptedCreation final : public InstanceCreationTask {
   public:
    explicit InterruptedCreation(bool cancel) : m_cancel(cancel) {}
    QString stagingPath() const { return m_stagingPath; }

   protected:
    std::unique_ptr<MinecraftInstance> createInstance() override
    {
        FS::write(FS::PathCombine(m_stagingPath, "minecraft/mods/complete.jar"), QByteArray("complete"));
        if (m_cancel)
            abort();
        else
            setError("download failed");
        return nullptr;
    }

   private:
    bool m_cancel;
};

class ModpackDownloadTest : public QObject {
    Q_OBJECT

   private slots:
    void abortDoesNotPublishPartialFile()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const auto path = root.filePath("cache/file");
        Net::FileSink sink(path);
        QNetworkRequest request(QUrl("http://127.0.0.1/file"));
        QCOMPARE(sink.init(request), Task::State::Running);
        QByteArray partial("partial");
        QCOMPARE(sink.write(partial), Task::State::Running);
        sink.abort();
        QVERIFY(!QFileInfo::exists(path));
        QVERIFY(QDir(QFileInfo(path).absolutePath()).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty());
    }

    void abortPreservesExistingCompleteFile()
    {
        QTemporaryDir root;
        const auto path = root.filePath("file");
        FS::write(path, QByteArray("complete"));
        Net::FileSink sink(path);
        QNetworkRequest request(QUrl("http://127.0.0.1/file"));
        QCOMPARE(sink.init(request), Task::State::Running);
        QByteArray partial("partial");
        QCOMPARE(sink.write(partial), Task::State::Running);
        sink.abort();
        QCOMPARE(FS::read(path), QByteArray("complete"));
    }

    void invalidatedEntryCannotBeCacheHit()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const QByteArray bytes("content");
        const auto expected = QCryptographicHash::hash(bytes, QCryptographicHash::Sha512);
        const auto source = root.filePath("source");
        FS::write(source, bytes);
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected);
        QString error;
        QVERIFY(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha512, expected, &error));
        entry->setETag("old-etag");
        entry->setRemoteChangedTimestamp("old-date");
        Modpack::Cache::invalidate(entry);
        Net::MetaCacheSink sink(entry, new Net::ChecksumValidator(QCryptographicHash::Md5), true);
        QNetworkRequest request(QUrl("http://127.0.0.1/file"));
        QCOMPARE(sink.init(request), Task::State::Running);
        QVERIFY(!request.hasRawHeader("If-None-Match"));
        QVERIFY(!request.hasRawHeader("If-Modified-Since"));
        sink.abort();
        QCOMPARE(FS::read(entry->getFullPath()), bytes);
        QVERIFY(entry->isStale());
    }

    void checksumFailureDoesNotPublish()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const auto path = root.filePath("cache/file");
        Net::FileSink sink(path);
        sink.addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha512,
                                                     QCryptographicHash::hash(QByteArray("expected"), QCryptographicHash::Sha512)));
        QNetworkRequest request(QUrl("http://127.0.0.1/file"));
        QCOMPARE(sink.init(request), Task::State::Running);
        QByteArray wrong("wrong");
        QCOMPARE(sink.write(wrong), Task::State::Running);
        SuccessfulReply reply;
        QCOMPARE(sink.finalize(reply), Task::State::Failed);
        sink.abort();
        QVERIFY(!QFileInfo::exists(path));
    }

    void interruptedCreationCleansStaging_data()
    {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("failure") << false;
        QTest::newRow("cancellation") << true;
    }

    void interruptedCreationCleansStaging()
    {
        QFETCH(bool, cancel);
        QTemporaryDir root;
        QVERIFY(root.isValid());
        InstanceList instances(nullptr, root.filePath("instances"));
        auto* creation = new InterruptedCreation(cancel);
        std::unique_ptr<Task> staged(instances.wrapInstanceTask(creation));
        QSignalSpy succeeded(staged.get(), &Task::succeeded);
        QSignalSpy finished(staged.get(), &Task::finished);
        staged->start();
        QVERIFY(staged->isFinished());
        QCOMPARE(staged->getState(), cancel ? Task::State::AbortedByUser : Task::State::Failed);
        QCOMPARE(succeeded.count(), 0);
        QCOMPARE(finished.count(), 1);
        QVERIFY(!QFileInfo::exists(creation->stagingPath()));
        QCOMPARE(instances.rowCount(), 0);
    }
};

QTEST_GUILESS_MAIN(ModpackDownloadTest)
#include "ModpackDownload_test.moc"
