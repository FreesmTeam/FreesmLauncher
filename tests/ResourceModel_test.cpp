#include <QAbstractItemModelTester>
#include <QTest>
#include <QTimer>

#include <Json.h>

#include <ui/pages/modplatform/ResourceModel.h>

#include "DummyResourceAPI.h"

using ResourceDownload::ResourceModel;

#define EXEC_TASK(EXEC)                                                                 \
    QEventLoop loop;                                                                    \
                                                                                        \
    connect(model, &ResourceModel::dataChanged, &loop, &QEventLoop::quit);              \
                                                                                        \
    QTimer expire_timer;                                                                \
    expire_timer.callOnTimeout(&loop, &QEventLoop::quit);                               \
    expire_timer.setSingleShot(true);                                                   \
    expire_timer.start(4000);                                                           \
                                                                                        \
    EXEC;                                                                               \
    if (model->hasActiveSearchJob())                                                    \
        loop.exec();                                                                    \
                                                                                        \
    QVERIFY2(expire_timer.isActive(), "Timer has expired. The search never finished."); \
    expire_timer.stop();                                                                \
                                                                                        \
    disconnect(model, nullptr, &loop, nullptr)

class ResourceModelTest;

class DummyResourceModel : public ResourceModel {
    Q_OBJECT

    friend class ResourceModelTest;

   public:
    DummyResourceModel() : ResourceModel(new DummyResourceAPI) {}
    ~DummyResourceModel() {}

    [[nodiscard]] auto metaEntryBase() const -> QString override { return ""; }

    ResourceAPI::SearchArgs createSearchArguments() override { return {}; }
    ResourceAPI::VersionSearchArgs createVersionsArguments(QModelIndex&) override { return {}; }
    ResourceAPI::ProjectInfoArgs createInfoArguments(QModelIndex&) override { return {}; }

    QJsonArray documentToArray(QJsonDocument& doc) const override { return doc.object().value("hits").toArray(); }

    void loadIndexedPack(ModPlatform::IndexedPack& pack, QJsonObject& obj) override
    {
        pack.authors.append({ Json::requireString(obj, "author"), "" });
        pack.description = Json::requireString(obj, "description");
        pack.addonId = Json::requireString(obj, "project_id");
    }
};

class ResourceModelTest : public QObject {
    Q_OBJECT
   private slots:
    void test_abstract_item_model()
    {
        auto dummy = DummyResourceModel();
        auto tester = QAbstractItemModelTester(&dummy);
    }

    void test_search()
    {
        
    }
};

QTEST_GUILESS_MAIN(ResourceModelTest)

#include "ResourceModel_test.moc"

#include "moc_DummyResourceAPI.cpp"
