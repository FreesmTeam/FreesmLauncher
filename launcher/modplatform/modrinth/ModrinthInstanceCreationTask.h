#pragma once

#include <optional>

#include <QByteArray>
#include <QJsonObject>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QVector>

#include "BaseInstance.h"
#include "InstanceCreationTask.h"

class ModrinthCreationTask final : public InstanceCreationTask {
    Q_OBJECT
    struct File {
        QString path;

        QByteArray hash;
        QQueue<QUrl> downloads;
        bool required = true;
    };

   public:
    ModrinthCreationTask(const QString& stagingPath,
                         bool trustedSource,
                         SettingsObject* globalSettings,
                         QWidget* parent,
                         QString id,
                         QString versionId = {},
                         QString originalInstanceId = {})
        : m_parent(parent), m_trustedSource(trustedSource), m_managed_id(std::move(id)), m_managed_version_id(std::move(versionId))
    {
        setStagingPath(stagingPath);
        setParentSettings(globalSettings);

        m_original_instance_id = std::move(originalInstanceId);
    }

    bool abort() override;

    bool updateInstance() override;
    std::unique_ptr<MinecraftInstance> createInstance() override;

   protected:
    bool keepOnGameDownloadFailure() const override { return !shouldOverride(); }

   private:
    bool parseManifest(const QString&, std::vector<File>&, bool setInternalData = true, bool showOptionalDialog = true);

   private:
    QWidget* m_parent = nullptr;
    bool m_trustedSource;

    QString m_minecraft_version, m_fabric_version, m_quilt_version, m_forge_version, m_neoForge_version;
    QString m_managed_id, m_managed_version_id, m_managed_name;

    std::vector<File> m_files;
    Task::Ptr m_task;
    QJsonObject m_manifest;
    bool m_installingContent = false;

    std::optional<BaseInstance*> m_instance;

    QString m_root_path = "minecraft";
};
