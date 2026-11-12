// SPDX-License-Identifier: GPL-3.0-only
#include <QFile>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QTest>

#include "FileSystem.h"
#include "modplatform/helpers/ContentDownloadJob.h"
#include "net/CachedFileSink.h"

class Reply final : public QNetworkReply {
   public:
    explicit Reply(int status = 200)
    {
        setUrl(QUrl("http://localhost/file"));
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        open(QIODevice::ReadOnly);
        setFinished(true);
    }
    void abort() override {}

   protected:
    qint64 readData(char*, qint64) override { return -1; }
};

struct Files {
    QTemporaryDir root;
    HttpMetaCache cache{ root.filePath("index.json") };
    MetaEntryPtr entry;
    QString target{ root.filePath("instance/mods/file.jar") };
    QByteArray content{ "complete file" };
    Net::Checksum checksum{ QCryptographicHash::Sha256, QCryptographicHash::hash(content, QCryptographicHash::Sha256) };

    Files()
    {
        cache.addBase("general", root.filePath("cache"));
        entry = cache.resolveEntry("general", "content");
    }
    void seed()
    {
        FS::write(entry->getFullPath(), content);
        entry->setMD5Sum(QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex()));
        entry->setStale(false);
        cache.updateEntry(entry);
    }
};

class CachedDownload final : public Net::Download {
   public:
    explicit CachedDownload(Files* files)
    {
        m_sink = std::make_unique<Net::CachedFileSink>(files->cache, files->entry, files->target, files->checksum, true);
    }
};

class CachedFileSinkTest : public QObject {
    Q_OBJECT

   private slots:
    void cancellationSettlesQueuedCopiesBeforeStagingCleanup()
    {
        Files f;
        f.seed();
        FS::write(f.target, QByteArray("old target"));
        QNetworkAccessManager network;
        Modpack::ContentDownloadJob job("cancel queued cache hit", &network, 1);
        auto request = makeShared<CachedDownload>(&f);
        job.addNetAction(request);
        bool cancelled = false;
        connect(&job, &Task::progress, &job, [&] {
            if (!cancelled) {
                cancelled = true;
                job.abort();
            }
        });
        connect(&job, &Task::finished, &job, [&] { QDir(QFileInfo(f.target).absolutePath()).removeRecursively(); });
        job.start();
        QTRY_VERIFY(job.isFinished());
        QTest::qWait(20);
        QVERIFY(cancelled);
        QCOMPARE(job.getState(), Task::State::AbortedByUser);
        QVERIFY(!QFile::exists(f.target));
        QCOMPARE(FS::read(f.entry->getFullPath()), f.content);
    }

    void hitCopiesToIndependentTargets()
    {
        Files f;
        f.seed();
        for (const auto& path : { f.target, f.root.filePath("other/renamed.jar.disabled") }) {
            Net::CachedFileSink sink(f.cache, f.entry, path, f.checksum, true);
            QNetworkRequest request;
            QCOMPARE(sink.init(request), Task::State::Succeeded);
            QCOMPARE(FS::read(path), f.content);
        }
        QVERIFY(QFile::remove(f.target));
        QCOMPARE(FS::read(f.entry->getFullPath()), f.content);
        QCOMPARE(FS::read(f.root.filePath("other/renamed.jar.disabled")), f.content);
    }

    void downloadAndNotModifiedBothCopy()
    {
        Files f;
        {
            Net::CachedFileSink sink(f.cache, f.entry, f.target, f.checksum, true);
            QNetworkRequest request;
            QCOMPARE(sink.init(request), Task::State::Running);
            auto data = f.content;
            QCOMPARE(sink.write(data), Task::State::Running);
            Reply reply;
            QCOMPARE(sink.finalize(reply), Task::State::Succeeded);
            QCOMPARE(FS::read(f.entry->getFullPath()), f.content);
            QCOMPARE(FS::read(f.target), f.content);
            QVERIFY(!f.entry->isStale());
            QVERIFY(f.entry->isEternal());
        }
        QVERIFY(QFile::remove(f.target));
        f.entry->setStale(true);
        f.entry->setETag("valid-etag");
        Net::CachedFileSink sink(f.cache, f.entry, f.target, f.checksum, true);
        QNetworkRequest request;
        QCOMPARE(sink.init(request), Task::State::Running);
        QCOMPARE(request.rawHeader("If-None-Match"), QByteArray("valid-etag"));
        Reply reply(304);
        QCOMPARE(sink.finalize(reply), Task::State::Succeeded);
        QCOMPARE(FS::read(f.target), f.content);
    }

    void badCacheIsNotAHitOrConditionalRequest_data()
    {
        QTest::addColumn<bool>("stale");
        QTest::newRow("fresh") << false;
        QTest::newRow("stale") << true;
    }

    void badCacheIsNotAHitOrConditionalRequest()
    {
        QFETCH(bool, stale);
        Files f;
        f.seed();
        f.entry->setStale(stale);
        FS::write(f.target, QByteArray("old target"));
        FS::write(f.entry->getFullPath(), QByteArray(f.content.size(), '!'));
        f.entry->setETag("invalid-etag");
        f.entry->setRemoteChangedTimestamp("old-date");
        Net::CachedFileSink sink(f.cache, f.entry, f.target, f.checksum, true);
        QNetworkRequest request;
        QCOMPARE(sink.init(request), Task::State::Running);
        QVERIFY(!request.hasRawHeader("If-None-Match"));
        QVERIFY(!request.hasRawHeader("If-Modified-Since"));
        QCOMPARE(FS::read(f.target), QByteArray("old target"));
        auto data = f.content;
        QCOMPARE(sink.write(data), Task::State::Running);
        Reply reply;
        QCOMPARE(sink.finalize(reply), Task::State::Succeeded);
        QCOMPARE(FS::read(f.target), f.content);
    }

    void copyFailureKeepsCacheAndDoesNotAcceptCacheAsTarget_data()
    {
        QTest::addColumn<bool>("hit");
        QTest::newRow("cache-hit") << true;
        QTest::newRow("after-download") << false;
    }

    void copyFailureKeepsCacheAndDoesNotAcceptCacheAsTarget()
    {
        QFETCH(bool, hit);
        Files f;
        if (hit)
            f.seed();
        auto blocker = f.root.filePath("blocker");
        FS::write(blocker, QByteArray("old file"));
        Net::CachedFileSink sink(f.cache, f.entry, blocker + "/target", f.checksum, true);
        QNetworkRequest request;
        QCOMPARE(sink.init(request), hit ? Task::State::Failed : Task::State::Running);
        if (!hit) {
            auto data = f.content;
            QCOMPARE(sink.write(data), Task::State::Running);
            Reply reply;
            QCOMPARE(sink.finalize(reply), Task::State::Failed);
            sink.abort();
        }
        QVERIFY(!sink.failReason().isEmpty());
        QVERIFY(!f.entry->isStale());
        QVERIFY(!sink.hasLocalData());
        QCOMPARE(FS::read(f.entry->getFullPath()), f.content);
        QCOMPARE(FS::read(blocker), QByteArray("old file"));
    }

    void failedValidationAndAbortDoNotPublishPartialFiles()
    {
        Files f;
        FS::write(f.target, QByteArray("old target"));
        for (bool cancel : { false, true }) {
            Net::CachedFileSink sink(f.cache, f.entry, f.target, f.checksum, true);
            QNetworkRequest request;
            QCOMPARE(sink.init(request), Task::State::Running);
            QByteArray data("partial");
            QCOMPARE(sink.write(data), Task::State::Running);
            if (cancel) {
                sink.abort();
            } else {
                Reply reply;
                QCOMPARE(sink.finalize(reply), Task::State::Failed);
                sink.abort();
            }
            QVERIFY(!QFile::exists(f.entry->getFullPath()));
            QCOMPARE(FS::read(f.target), QByteArray("old target"));
            QVERIFY(QDir(f.root.filePath("cache")).entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot).isEmpty());
        }
    }
};

QTEST_GUILESS_MAIN(CachedFileSinkTest)
#include "CachedFileSink_test.moc"
