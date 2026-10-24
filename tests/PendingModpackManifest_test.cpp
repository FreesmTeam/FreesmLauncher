// SPDX-License-Identifier: GPL-3.0-only

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>

#include "modplatform/helpers/PendingModpackManifest.h"

class PendingModpackManifestTest : public QObject {
    Q_OBJECT

   private slots:
    void test_FilterModrinth()
    {
        const auto original = QJsonDocument::fromJson(R"({"files":[
            {"path":"mods/done.jar"}, {"path":"mods/failed.jar"}, {"path":"mods/not-started.jar"}
        ]})")
                                  .object();
        Modpack::PendingManifest manifest(Modpack::Format::Modrinth, original);
        manifest.markCompleted("mods/done.jar");
        manifest.markCompleted("mods/done.jar");
        manifest.markCompleted("unknown.jar");

        const auto expected = QJsonDocument::fromJson(R"({"files":[
            {"path":"mods/failed.jar"}, {"path":"mods/not-started.jar"}
        ]})")
                                  .object();
        QCOMPARE(manifest.remaining(), expected);
        QCOMPARE(original.value("files").toArray().size(), 3);
    }

    void test_FilterCurseForge()
    {
        const auto original = QJsonDocument::fromJson(R"({"files":[
            {"projectID":12,"fileID":34,"required":true},
            {"projectID":12,"fileID":35,"required":false},
            {"projectID":56,"fileID":34,"required":true}
        ]})")
                                  .object();
        Modpack::PendingManifest manifest(Modpack::Format::CurseForge, original);
        manifest.markCompleted("12:34");
        const auto expected = QJsonDocument::fromJson(R"({"files":[
            {"projectID":12,"fileID":35,"required":false},
            {"projectID":56,"fileID":34,"required":true}
        ]})")
                                  .object();
        QCOMPARE(manifest.remaining(), expected);
    }

    void test_PreserveOriginalFields()
    {
        const auto original = QJsonDocument::fromJson(R"({
            "formatVersion":1,"name":"Pack","dependencies":{"minecraft":"1.21.1","fabric-loader":"0.16.0"},
            "unknown":{"nested":[1,true,null]},"files":[
                {"path":"mods/done.jar"},
                {"path":"mods/optional.jar","env":{"client":"optional","server":"unsupported"},
                 "downloads":["https:\/\/first.example/mod.jar","https:\/\/backup.example/mod.jar"],
                 "hashes":{"sha512":"original-hash"},"fileSize":42,"unknown":[false,{"keep":"me"}]}
            ]
        })")
                                  .object();
        Modpack::PendingManifest manifest(Modpack::Format::Modrinth, original);
        manifest.markCompleted("mods/done.jar");
        auto expected = original;
        expected.insert("files", QJsonArray{ original.value("files").toArray().at(1) });
        QCOMPARE(manifest.remaining(), expected);

        const auto curseForge = QJsonDocument::fromJson(R"({
            "manifestType":"minecraftModpack","minecraft":{"version":"1.21.1","modLoaders":[{"id":"neoforge-21.1"}]},
            "overrides":"overrides","unknown":true,
            "files":[{"projectID":1,"fileID":2,"required":false,"unknown":{"keep":true}}]
        })")
                                    .object();
        Modpack::PendingManifest cfManifest(Modpack::Format::CurseForge, curseForge);
        QCOMPARE(cfManifest.remaining(), curseForge);
    }

    void test_InvalidAndEmptyManifest()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const auto path = temporary.filePath("manifest.json");
        QString error;
        for (const auto format : { Modpack::Format::Modrinth, Modpack::Format::CurseForge }) {
            Modpack::PendingManifest empty(format, QJsonObject{ { "files", QJsonArray{} } });
            QVERIFY2(empty.save(path, &error), qPrintable(error));
            const auto loaded = Modpack::PendingManifest::load(format, path, &error);
            QVERIFY2(loaded.has_value(), qPrintable(error));
            QCOMPARE(loaded->remaining(), QJsonObject({ { "files", QJsonArray{} } }));

            const QList<QByteArray> invalid{ "{broken", "[]", "{}", R"({"files":null})", R"({"files":{}})", R"({"files":"not an array"})" };
            for (const auto& data : invalid) {
                QFile file(path);
                QVERIFY(file.open(QIODevice::WriteOnly));
                QCOMPARE(file.write(data), data.size());
                file.close();
                error.clear();
                QVERIFY(!Modpack::PendingManifest::load(format, path, &error).has_value());
                QVERIFY(!error.isEmpty());
            }

            error.clear();
            QVERIFY(!Modpack::PendingManifest::load(format, temporary.filePath("missing.json"), &error).has_value());
            QVERIFY(!error.isEmpty());

            Modpack::PendingManifest invalidManifest(format, QJsonObject{ { "files", "invalid" } });
            QCOMPARE(invalidManifest.remaining().value("files").toString(), QString("invalid"));
            QFile oldFile(path);
            QVERIFY(oldFile.open(QIODevice::ReadOnly));
            const auto oldData = oldFile.readAll();
            oldFile.close();
            error.clear();
            QVERIFY(!invalidManifest.save(path, &error));
            QVERIFY(!error.isEmpty());
            QVERIFY(oldFile.open(QIODevice::ReadOnly));
            QCOMPARE(oldFile.readAll(), oldData);
        }
    }

    void test_SaveAndReload()
    {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QCOMPARE(Modpack::pendingManifestPath(temporary.path(), Modpack::Format::Modrinth),
                 temporary.filePath("mrpack/remaining.index.json"));
        QCOMPARE(Modpack::pendingManifestPath(temporary.path(), Modpack::Format::CurseForge),
                 temporary.filePath("flame/remaining.manifest.json"));

        const auto original = QJsonDocument::fromJson(R"({"name":"Pack","files":[
            {"path":"mods/a.jar","projectID":1,"fileID":2},
            {"path":"mods/b.jar","projectID":3,"fileID":4}
        ]})")
                                  .object();
        for (const auto format : { Modpack::Format::Modrinth, Modpack::Format::CurseForge }) {
            const auto path = Modpack::pendingManifestPath(temporary.path(), format);
            QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
            Modpack::PendingManifest manifest(format, original);
            QString error;
            QVERIFY2(manifest.save(path, &error), qPrintable(error));
            QFile oldFile(path);
            QVERIFY(oldFile.open(QIODevice::ReadOnly));
            const auto oldData = oldFile.readAll();
            oldFile.close();

            manifest.markCompleted(format == Modpack::Format::Modrinth ? "mods/a.jar" : "1:2");
            // A regular file cannot serve as a parent directory, regardless of write permissions.
            QVERIFY(!manifest.save(QDir(path).filePath("remaining.json"), &error));
            QVERIFY(!error.isEmpty());
            QVERIFY(oldFile.open(QIODevice::ReadOnly));
            QCOMPARE(oldFile.readAll(), oldData);
            oldFile.close();

            QVERIFY2(manifest.save(path, &error), qPrintable(error));
            auto loaded = Modpack::PendingManifest::load(format, path, &error);
            QVERIFY2(loaded.has_value(), qPrintable(error));
            const auto expected = QJsonDocument::fromJson(R"({"name":"Pack","files":[
                {"path":"mods/b.jar","projectID":3,"fileID":4}
            ]})")
                                      .object();
            QCOMPARE(loaded->remaining(), expected);
            loaded->markCompleted(format == Modpack::Format::Modrinth ? "mods/b.jar" : "3:4");
            QVERIFY2(loaded->save(path, &error), qPrintable(error));
            const auto completed = Modpack::PendingManifest::load(format, path, &error);
            QVERIFY2(completed.has_value(), qPrintable(error));
            QCOMPARE(completed->remaining(), QJsonObject({ { "name", "Pack" }, { "files", QJsonArray{} } }));
            QVERIFY(!manifest.save(temporary.filePath("missing/manifest.json"), nullptr));
        }
    }
};

QTEST_GUILESS_MAIN(PendingModpackManifestTest)

#include "PendingModpackManifest_test.moc"
