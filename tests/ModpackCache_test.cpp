// SPDX-License-Identifier: GPL-3.0-only
#include <QDateTime>
#include <QFile>
#include <QNetworkAccessManager>
#include <QTemporaryDir>
#include <QTest>

#include "FileSystem.h"
#include "modplatform/helpers/ModpackCache.h"
#include "net/ApiDownload.h"

class CountingNetworkManager final : public QNetworkAccessManager {
   public:
    int requests = 0;

   protected:
    QNetworkReply* createRequest(Operation operation, const QNetworkRequest& request, QIODevice* data = nullptr) override
    {
        ++requests;
        return QNetworkAccessManager::createRequest(operation, request, data);
    }
};

class ModpackCacheTest : public QObject {
    Q_OBJECT

   private slots:
    void reusesCompleteFiles_data()
    {
        QTest::addColumn<int>("algorithm");
        QTest::newRow("sha512") << int(QCryptographicHash::Sha512);
        QTest::newRow("sha1") << int(QCryptographicHash::Sha1);
        QTest::newRow("md5") << int(QCryptographicHash::Md5);
    }

    void reusesCompleteFiles()
    {
        QFETCH(int, algorithm);
        const auto algo = static_cast<QCryptographicHash::Algorithm>(algorithm);
        QTemporaryDir root;
        QVERIFY(root.isValid());
        HttpMetaCache cache(root.filePath("metacache"));
        cache.addBase("general", root.filePath("cache"));
        const QByteArray bytes("mod-content");
        const auto expected = QCryptographicHash::hash(bytes, algo);
        const auto platform =
            algo == QCryptographicHash::Sha512 ? Modpack::Cache::Platform::Modrinth : Modpack::Cache::Platform::CurseForge;
        const auto source = root.filePath("download.jar");
        FS::write(source, bytes);
        auto entry = Modpack::Cache::resolveEntry(cache, platform, algo, expected);
        QVERIFY(entry);
        QVERIFY(entry->isStale());
        QString error;
        QVERIFY2(Modpack::Cache::storeFile(cache, entry, source, algo, expected, &error), qPrintable(error));
        QVERIFY(QFileInfo::exists(source));
        const auto enabled = root.filePath("instance-one/mods/a.jar");
        const auto disabled = root.filePath("instance-two/mods/b.jar.disabled");
        QVERIFY2(Modpack::Cache::copyVerified(entry->getFullPath(), enabled, algo, expected, &error), qPrintable(error));
        QVERIFY2(Modpack::Cache::copyVerified(entry->getFullPath(), disabled, algo, expected, &error), qPrintable(error));
        FS::write(enabled, QByteArray("user-edit"));
        QCOMPARE(FS::read(disabled), bytes);
        QCOMPARE(FS::read(entry->getFullPath()), bytes);
        QVERIFY(FS::deletePath(root.filePath("instance-one")));
        auto retry = Modpack::Cache::resolveEntry(cache, platform, algo, expected);
        QVERIFY(!retry->isStale());
        QCOMPARE(retry->getFullPath(), entry->getFullPath());
        QVERIFY(retry->isEternal());
        QCOMPARE(retry->getMD5Sum(), QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex()));
    }

    void distinctContentsDoNotCollide()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.path());
        const auto first = QCryptographicHash::hash(QByteArray("first"), QCryptographicHash::Sha1);
        const auto second = QCryptographicHash::hash(QByteArray("second"), QCryptographicHash::Sha1);
        auto a = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha1, first);
        auto b = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, first);
        auto c = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, second);
        QVERIFY(a->getFullPath() != b->getFullPath());
        QVERIFY(b->getFullPath() != c->getFullPath());
    }

    void rejectsInvalidHashes()
    {
        HttpMetaCache cache;
        cache.addBase("general", QDir::tempPath());
        QVERIFY(!Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, {}));
        QVERIFY(!Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, QByteArray("short")));
        QVERIFY(
            !Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha256, QByteArray(32, 'x')));
    }

    void rejectsWrongHashWithoutPublishing()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const auto expected = QCryptographicHash::hash(QByteArray("good"), QCryptographicHash::Sha1);
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, expected);
        const auto source = root.filePath("bad.jar");
        FS::write(source, QByteArray("bad"));
        QString error;
        QVERIFY(!Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha1, expected, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(entry->isStale());
        QVERIFY(!QFileInfo::exists(entry->getFullPath()));
    }

    void corruptionClearsConditionalHeaders()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const QByteArray bytes("good");
        const auto expected = QCryptographicHash::hash(bytes, QCryptographicHash::Sha512);
        const auto source = root.filePath("source");
        FS::write(source, bytes);
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected);
        QString error;
        QVERIFY2(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha512, expected, &error), qPrintable(error));
        entry->setETag("old-etag");
        entry->setRemoteChangedTimestamp("old-date");
        const auto modified = QFileInfo(entry->getFullPath()).lastModified();
        QFile corrupt(entry->getFullPath());
        QVERIFY(corrupt.open(QIODevice::ReadWrite));
        QCOMPARE(corrupt.write("evil", 4), qint64(4));
        QVERIFY(corrupt.flush());
        QVERIFY(corrupt.setFileTime(modified, QFileDevice::FileModificationTime));
        corrupt.close();
        QCOMPARE(QFileInfo(entry->getFullPath()).lastModified(), modified);
        auto resolved = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected);
        QVERIFY(resolved->isStale());
        QVERIFY(resolved->getETag().isEmpty());
        QVERIFY(resolved->getRemoteChangedTimestamp().isEmpty());
    }

    void persistsAcrossNormalRestart()
    {
        QTemporaryDir root;
        const auto expected = QCryptographicHash::hash(QByteArray("content"), QCryptographicHash::Sha1);
        const auto source = root.filePath("source");
        FS::write(source, QByteArray("content"));
        {
            HttpMetaCache cache(root.filePath("metacache"));
            cache.addBase("general", root.filePath("cache"));
            auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, expected);
            QString error;
            QVERIFY2(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha1, expected, &error), qPrintable(error));
            cache.SaveNow();
        }
        HttpMetaCache cache(root.filePath("metacache"));
        cache.addBase("general", root.filePath("cache"));
        cache.Load();
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Sha1, expected);
        QVERIFY(!entry->isStale());
        QVERIFY(entry->isEternal());
    }

    void missingAndClearedFilesAreMisses()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const auto expected = QCryptographicHash::hash(QByteArray("content"), QCryptographicHash::Md5);
        const auto source = root.filePath("source");
        FS::write(source, QByteArray("content"));
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Md5, expected);
        QString error;
        QVERIFY(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Md5, expected, &error));
        QVERIFY(QFile::remove(entry->getFullPath()));
        entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Md5, expected);
        QVERIFY(entry->isStale());
        QVERIFY(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Md5, expected, &error));
        QVERIFY(cache.evictAll());
        QVERIFY(Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, QCryptographicHash::Md5, expected)->isStale());
    }

    void failedCopyPreservesDestinationAndCache()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const QByteArray bytes("content");
        const auto expected = QCryptographicHash::hash(bytes, QCryptographicHash::Sha512);
        const auto source = root.filePath("source");
        const auto target = root.filePath("target");
        FS::write(source, bytes);
        FS::write(target, QByteArray("old-target"));
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected);
        QString error;
        QVERIFY(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha512, expected, &error));
        QVERIFY(!Modpack::Cache::copyVerified(entry->getFullPath(), target, QCryptographicHash::Sha512, QByteArray(64, 'x'), &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(FS::read(target), QByteArray("old-target"));
        QCOMPARE(FS::read(entry->getFullPath()), bytes);
        QVERIFY(!Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected)->isStale());
        QVERIFY(!Modpack::Cache::copyVerified(entry->getFullPath(), root.path(), QCryptographicHash::Sha512, expected, &error));
        QVERIFY(Modpack::Cache::matchesHash(entry->getFullPath(), QCryptographicHash::Sha512, expected));
    }

    void cacheHitDoesNotRequestNetwork()
    {
        QTemporaryDir root;
        HttpMetaCache cache;
        cache.addBase("general", root.filePath("cache"));
        const auto source = root.filePath("source");
        FS::write(source, QByteArray("content"));
        const auto expected = QCryptographicHash::hash(QByteArray("content"), QCryptographicHash::Sha512);
        auto entry = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::Modrinth, QCryptographicHash::Sha512, expected);
        QString error;
        QVERIFY(Modpack::Cache::storeFile(cache, entry, source, QCryptographicHash::Sha512, expected, &error));
        CountingNetworkManager network;
        Net::ModrinthDownloadMeta meta{ .reason = "modpack", .gameVersion = "1.20.1", .loader = "fabric" };
        auto download =
            Net::ApiDownload::makeCached(QUrl("http://127.0.0.1:1/unreachable"), entry, Net::Download::Option::MakeEternal, meta);
        download->setNetwork(&network);
        download->start();
        QVERIFY(download->wasSuccessful());
        QCOMPARE(network.requests, 0);
    }
};

QTEST_GUILESS_MAIN(ModpackCacheTest)
#include "ModpackCache_test.moc"
