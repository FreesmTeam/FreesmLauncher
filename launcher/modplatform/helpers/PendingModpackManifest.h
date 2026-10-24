// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QJsonObject>
#include <QSet>
#include <QString>

#include <optional>

namespace Modpack {

enum class Format { Modrinth, CurseForge };
enum class InstallMode { Create, Resume };

QString pendingManifestPath(const QString& instanceRoot, Format format);

class PendingManifest {
   public:
    PendingManifest(Format format, QJsonObject original);

    QJsonObject remaining() const;
    void markCompleted(const QString& key);
    bool save(const QString& path, QString* error) const;
    static std::optional<PendingManifest> load(Format format, const QString& path, QString* error);

   private:
    Format m_format;
    QJsonObject m_original;
    QSet<QString> m_completed;
};

}  // namespace Modpack
