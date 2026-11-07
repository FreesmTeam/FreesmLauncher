// SPDX-License-Identifier: GPL-3.0-only
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

#include "modplatform/helpers/ModpackCache.h"
#include "net/ApiDownload.h"

class ModpackPlatformCacheTest : public QObject {
    Q_OBJECT

   private slots:
    void reuse_data()
    {
        QTest::addColumn<int>("platform");
        QTest::addColumn<QString>("name");
        QTest::addColumn<int>("algorithm");
        QTest::addColumn<QString>("algorithmName");
        QTest::newRow("atlauncher-mod") << int(Modpack::Cache::Platform::ATLauncher) << QString("atlauncher")
                                        << int(QCryptographicHash::Md5) << QString("md5");
        QTest::newRow("atlauncher-config") << int(Modpack::Cache::Platform::ATLauncher) << QString("atlauncher")
                                           << int(QCryptographicHash::Sha1) << QString("sha1");
        QTest::newRow("ftb") << int(Modpack::Cache::Platform::FTB) << QString("ftb") << int(QCryptographicHash::Sha1) << QString("sha1");
        QTest::newRow("technic-solder") << int(Modpack::Cache::Platform::Technic) << QString("technic") << int(QCryptographicHash::Md5)
                                        << QString("md5");
    }

    void reuse()
    {
        QFETCH(int, platform);
        QFETCH(QString, name);
        QFETCH(int, algorithm);
        QFETCH(QString, algorithmName);
        const auto hashAlgorithm = QCryptographicHash::Algorithm(algorithm);
        const auto cachePlatform = Modpack::Cache::Platform(platform);
        const QByteArray content("complete modpack file");
        const auto expected = QCryptographicHash::hash(content, hashAlgorithm);

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        HttpMetaCache cache(temp.filePath("index.json"));
        cache.addBase("general", temp.filePath("cache"));
        QVERIFY(!Modpack::Cache::resolveEntry(cache, cachePlatform, hashAlgorithm, {}));
        auto entry = Modpack::Cache::resolveEntry(cache, cachePlatform, hashAlgorithm, expected);
        QVERIFY(entry);
        QCOMPARE(entry->getFullPath(),
                 temp.filePath(QString("cache/modpack-files/%1/%2/%3").arg(name, algorithmName, QString::fromLatin1(expected.toHex()))));
        const auto other = Modpack::Cache::resolveEntry(cache, Modpack::Cache::Platform::CurseForge, hashAlgorithm, expected);
        QVERIFY(entry->getFullPath() != other->getFullPath());

        QFile source(temp.filePath("source"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(content), content.size());
        source.close();
        QString error;
        QVERIFY2(Modpack::Cache::storeFile(cache, entry, source.fileName(), hashAlgorithm, expected, &error), qPrintable(error));
        entry = Modpack::Cache::resolveEntry(cache, cachePlatform, hashAlgorithm, expected);
        QVERIFY(!entry->isStale());
        QVERIFY(entry->isEternal());
        auto download = Net::ApiDownload::makeCached(QUrl("http://127.0.0.1:1/unreachable"), entry, Net::Download::Option::MakeEternal);
        download->start();
        QVERIFY(download->wasSuccessful());

        const auto first = temp.filePath("first/mods/one.jar");
        const auto second = temp.filePath("second/mods/two.jar.disabled");
        QVERIFY2(Modpack::Cache::copyVerified(entry->getFullPath(), first, hashAlgorithm, expected, &error), qPrintable(error));
        QVERIFY2(Modpack::Cache::copyVerified(entry->getFullPath(), second, hashAlgorithm, expected, &error), qPrintable(error));
        QVERIFY(QFile::remove(first));
        QVERIFY(Modpack::Cache::matchesHash(second, hashAlgorithm, expected));
        QVERIFY(Modpack::Cache::matchesHash(entry->getFullPath(), hashAlgorithm, expected));

        entry->setETag("old-etag");
        entry->setRemoteChangedTimestamp("old-timestamp");
        QFile damaged(entry->getFullPath());
        const auto modified = QFileInfo(damaged).lastModified();
        QVERIFY(damaged.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(damaged.write(QByteArray(content.size(), '!')), content.size());
        QVERIFY(damaged.flush());
        QVERIFY(damaged.setFileTime(modified, QFileDevice::FileModificationTime));
        damaged.close();
        entry = Modpack::Cache::resolveEntry(cache, cachePlatform, hashAlgorithm, expected);
        QVERIFY(entry->isStale());
        QVERIFY(entry->getETag().isEmpty());
        QVERIFY(entry->getRemoteChangedTimestamp().isEmpty());
    }
};

QTEST_GUILESS_MAIN(ModpackPlatformCacheTest)
#include "ModpackPlatformCache_test.moc"
