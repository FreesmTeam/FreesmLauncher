// SPDX-License-Identifier: GPL-3.0-only
#include "ModpackCache.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QSaveFile>

#include "FileSystem.h"

namespace Modpack::Cache {
bool matchesHash(const QString& path, QCryptographicHash::Algorithm algorithm, const QByteArray& expected)
{
    if (expected.isEmpty() || expected.size() != QCryptographicHash::hashLength(algorithm))
        return false;
    QFile file(path);
    QCryptographicHash hash(algorithm);
    return QFileInfo(path).isFile() && file.open(QIODevice::ReadOnly) && hash.addData(&file) && hash.result() == expected;
}

void invalidate(const MetaEntryPtr& entry)
{
    entry->setStale(true);
    entry->setETag({});
    entry->setRemoteChangedTimestamp({});
}

MetaEntryPtr resolveEntry(HttpMetaCache& cache, Platform platform, QCryptographicHash::Algorithm algorithm, const QByteArray& expected)
{
    QString algorithmName;
    switch (algorithm) {
        case QCryptographicHash::Sha512:
            algorithmName = QStringLiteral("sha512");
            break;
        case QCryptographicHash::Sha1:
            algorithmName = QStringLiteral("sha1");
            break;
        case QCryptographicHash::Md5:
            algorithmName = QStringLiteral("md5");
            break;
        default:
            return {};
    }
    if (expected.isEmpty() || expected.size() != QCryptographicHash::hashLength(algorithm))
        return {};
    QString platformName;
    switch (platform) {
        case Platform::Modrinth:
            platformName = QStringLiteral("modrinth");
            break;
        case Platform::CurseForge:
            platformName = QStringLiteral("curseforge");
            break;
        case Platform::ATLauncher:
            platformName = QStringLiteral("atlauncher");
            break;
        case Platform::FTB:
            platformName = QStringLiteral("ftb");
            break;
        case Platform::Technic:
            platformName = QStringLiteral("technic");
            break;
        default:
            return {};
    }
    const auto key = QStringLiteral("modpack-files/%1/%2/%3").arg(platformName, algorithmName, QString::fromLatin1(expected.toHex()));
    auto entry = cache.resolveEntry(QStringLiteral("general"), key);
    if (!entry->isStale() && !matchesHash(entry->getFullPath(), algorithm, expected))
        invalidate(entry);
    return entry;
}

bool copyVerified(const QString& source,
                  const QString& destination,
                  QCryptographicHash::Algorithm algorithm,
                  const QByteArray& expected,
                  QString* error)
{
    error->clear();
    if (expected.isEmpty() || expected.size() != QCryptographicHash::hashLength(algorithm)) {
        *error = QObject::tr("Invalid expected hash for %1.").arg(source);
        return false;
    }
    QFile input(source);
    if (!input.open(QIODevice::ReadOnly)) {
        *error = QObject::tr("Could not read %1: %2").arg(source, input.errorString());
        return false;
    }
    if (!FS::ensureFilePathExists(destination)) {
        *error = QObject::tr("Could not create the parent directory for %1.").arg(destination);
        return false;
    }
    QSaveFile output(destination);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) {
        *error = QObject::tr("Could not write %1: %2").arg(destination, output.errorString());
        return false;
    }
    QCryptographicHash hasher(algorithm);
    QByteArray buffer(64 * 1024, '\0');
    for (;;) {
        const auto count = input.read(buffer.data(), buffer.size());
        if (count < 0) {
            *error = QObject::tr("Could not read %1: %2").arg(source, input.errorString());
            return false;
        }
        if (count == 0)
            break;
        hasher.addData(QByteArrayView(buffer.constData(), count));
        if (output.write(buffer.constData(), count) != count) {
            *error = QObject::tr("Could not write %1: %2").arg(destination, output.errorString());
            return false;
        }
    }
    if (hasher.result() != expected) {
        *error = QObject::tr("File failed hash validation: %1").arg(source);
        return false;
    }
    if (!output.commit()) {
        *error = QObject::tr("Could not commit file %1: %2").arg(destination, output.errorString());
        return false;
    }
    return true;
}

bool storeFile(HttpMetaCache& cache,
               const MetaEntryPtr& entry,
               const QString& source,
               QCryptographicHash::Algorithm algorithm,
               const QByteArray& expected,
               QString* error)
{
    error->clear();
    const auto path = entry->getFullPath();
    if (QFileInfo(source).absoluteFilePath() != QFileInfo(path).absoluteFilePath()) {
        if (!copyVerified(source, path, algorithm, expected, error))
            return false;
    } else if (!matchesHash(path, algorithm, expected)) {
        invalidate(entry);
        *error = QObject::tr("File failed hash validation: %1").arg(path);
        return false;
    }
    QFile cached(path);
    QCryptographicHash md5(QCryptographicHash::Md5);
    if (!cached.open(QIODevice::ReadOnly) || !md5.addData(&cached)) {
        invalidate(entry);
        *error = QObject::tr("Could not read cached file: %1").arg(path);
        return false;
    }
    cached.close();
    entry->setMD5Sum(QString::fromLatin1(md5.result().toHex()));
    entry->setLocalChangedTimestamp(QFileInfo(path).lastModified().toUTC().toMSecsSinceEpoch());
    entry->setETag({});
    entry->setRemoteChangedTimestamp({});
    entry->makeEternal(true);
    entry->setStale(false);
    if (!cache.updateEntry(entry)) {
        invalidate(entry);
        *error = QObject::tr("Could not register cached file: %1").arg(path);
        return false;
    }
    return true;
}

}  // namespace Modpack::Cache
