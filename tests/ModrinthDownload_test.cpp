// SPDX-License-Identifier: GPL-3.0-only
#include <QTest>
#include <type_traits>

#include "BuildConfig.h"
#include "modplatform/modrinth/ModrinthDownload.h"
#include "net/ApiDownload.h"

class ModrinthDownloadTest : public QObject {
    Q_OBJECT

   private slots:
    void sendsDownloadMetadata()
    {
        Modrinth::DownloadHeaderProxy proxy({ .reason = "modpack", .gameVersion = "1.20.1", .loader = "fabric" });
        QNetworkRequest request(QUrl(QString("https://%1/file.jar").arg(BuildConfig.MODRINTH_DOWNLOAD_HOST)));
        proxy.writeHeaders(request);
        QVERIFY(request.hasRawHeader("modrinth-download-meta"));
        const QJsonObject expected{ { "reason", "modpack" }, { "game_version", "1.20.1" }, { "loader", "fabric" } };
        QCOMPARE(QJsonDocument::fromJson(request.rawHeader("modrinth-download-meta")).object(), expected);
    }

    void omitsEmptyOptionalFields()
    {
        Modrinth::DownloadHeaderProxy proxy({ .reason = "update" });
        QNetworkRequest request(QUrl(QString("https://%1/file.jar").arg(BuildConfig.MODRINTH_DOWNLOAD_HOST)));
        proxy.writeHeaders(request);
        const QJsonObject expected{ { "reason", "update" } };
        QCOMPARE(QJsonDocument::fromJson(request.rawHeader("modrinth-download-meta")).object(), expected);
    }

    void emptyReasonDoesNotSendMetadata()
    {
        Modrinth::DownloadHeaderProxy proxy({ .reason = {}, .gameVersion = "1.20.1", .loader = "fabric" });
        QNetworkRequest request(QUrl(QString("https://%1/file.jar").arg(BuildConfig.MODRINTH_DOWNLOAD_HOST)));
        proxy.writeHeaders(request);
        QVERIFY(!request.hasRawHeader("modrinth-download-meta"));
    }

    void doesNotSendMetadataToOtherHosts_data()
    {
        QTest::addColumn<QUrl>("url");
        QTest::newRow("flame-api") << QUrl(BuildConfig.FLAME_BASE_URL);
        QTest::newRow("flame-download") << QUrl(QString("https://%1/file.jar").arg(BuildConfig.FLAME_DOWNLOAD_HOST));
        QTest::newRow("modrinth-api") << QUrl(BuildConfig.MODRINTH_PROD_URL);
        QTest::newRow("modrinth-staging-api") << QUrl(BuildConfig.MODRINTH_STAGING_URL);
        QTest::newRow("fallback") << QUrl("https://example.invalid/file.jar");
        QTest::newRow("lookalike") << QUrl(QString("https://%1.example.invalid/file.jar").arg(BuildConfig.MODRINTH_DOWNLOAD_HOST));
    }

    void doesNotSendMetadataToOtherHosts()
    {
        QFETCH(QUrl, url);
        Modrinth::DownloadHeaderProxy proxy({ .reason = "modpack", .gameVersion = "1.20.1", .loader = "fabric" });
        QNetworkRequest request(url);
        proxy.writeHeaders(request);
        QVERIFY(!request.hasRawHeader("modrinth-download-meta"));
    }

    void cachedDownloadHasNoPlatformParameter()
    {
        using Factory = Net::Download::Ptr (*)(QUrl, MetaEntryPtr, Net::Download::Options);
        QVERIFY((std::is_same_v<decltype(&Net::ApiDownload::makeCached), Factory>));
    }

    void fileDownloadHasNoPlatformParameter()
    {
        using Factory = Net::Download::Ptr (*)(QUrl, QString, Net::Download::Options);
        QVERIFY((std::is_same_v<decltype(&Net::ApiDownload::makeFile), Factory>));
    }
};

QTEST_GUILESS_MAIN(ModrinthDownloadTest)
#include "ModrinthDownload_test.moc"
