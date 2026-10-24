// SPDX-License-Identifier: GPL-3.0-only

#include "PendingModpackManifest.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QObject>
#include <QSaveFile>

#include <utility>

namespace Modpack {

QString pendingManifestPath(const QString& instanceRoot, Format format)
{
    return QDir(instanceRoot).filePath(format == Format::Modrinth ? "mrpack/remaining.index.json" : "flame/remaining.manifest.json");
}

PendingManifest::PendingManifest(Format format, QJsonObject original) : m_format(format), m_original(std::move(original)) {}

QJsonObject PendingManifest::remaining() const
{
    auto result = m_original;
    if (!m_original.value("files").isArray())
        return result;

    QJsonArray files;
    for (const auto& value : m_original.value("files").toArray()) {
        const auto file = value.toObject();
        QString key;
        if (m_format == Format::Modrinth) {
            key = file.value("path").toString();
        } else if (file.value("projectID").isDouble() && file.value("fileID").isDouble()) {
            key = QString::number(file.value("projectID").toInteger()) + ':' + QString::number(file.value("fileID").toInteger());
        }
        if (key.isEmpty() || !m_completed.contains(key))
            files.append(value);
    }
    result.insert("files", files);
    return result;
}

void PendingManifest::markCompleted(const QString& key)
{
    m_completed.insert(key);
}

bool PendingManifest::save(const QString& path, QString* error) const
{
    if (error)
        error->clear();
    if (!m_original.value("files").isArray()) {
        if (error)
            *error = QObject::tr("The modpack manifest must contain a files array.");
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const auto data = QJsonDocument(remaining()).toJson();
    if (file.write(data) != data.size()) {
        if (error)
            *error = file.errorString();
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

std::optional<PendingManifest> PendingManifest::load(Format format, const QString& path, QString* error)
{
    if (error)
        error->clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return std::nullopt;
    }
    const auto data = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (error)
            *error = file.errorString();
        return std::nullopt;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error)
            *error = parseError.errorString();
        return std::nullopt;
    }
    if (!document.isObject() || !document.object().value("files").isArray()) {
        if (error)
            *error = QObject::tr("The modpack manifest must be an object containing a files array.");
        return std::nullopt;
    }
    return PendingManifest(format, document.object());
}

}  // namespace Modpack
