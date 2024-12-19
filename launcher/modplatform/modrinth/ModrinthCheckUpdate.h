#pragma once

#include "modplatform/CheckUpdateTask.h"

class ModrinthCheckUpdate : public CheckUpdateTask {
    Q_OBJECT

   public:
    ModrinthCheckUpdate(QList<Resource*>& resources,
                        std::list<Version>& mcVersions,
                        QList<ModPlatform::ModLoaderType> loadersList,
                        std::shared_ptr<ResourceFolderModel> resourceModel)
        : CheckUpdateTask(resources, mcVersions, std::move(loadersList), std::move(resourceModel))
        , m_hashType(ModPlatform::ProviderCapabilities::hashType(ModPlatform::ResourceProvider::MODRINTH).first())
    {}

   public slots:
    bool abort() override;

   protected slots:
    void executeTask() override;
    void getUpdateModsForLoader(std::optional<ModPlatform::ModLoaderTypes> loader);
    void checkVersionsResponse(std::shared_ptr<QByteArray> response, std::optional<ModPlatform::ModLoaderTypes> loader);
    void checkNextLoader();

   private:
    Task::Ptr m_job = nullptr;
    QHash<QString, Resource*> m_mappings;
    QString m_hashType;
    int m_loaderIdx = 0;
};
