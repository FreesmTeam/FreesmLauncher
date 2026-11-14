// SPDX-License-Identifier: GPL-3.0-only
#include "ModpackCache.h"

#include <QFile>
#include <QFileInfo>

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
        default:
            return {};
    }
    const auto key = QStringLiteral("modpack-files/%1/%2/%3").arg(platformName, algorithmName, QString::fromLatin1(expected.toHex()));
    auto entry = cache.resolveEntry(QStringLiteral("general"), key);
    if (!entry->isStale() && !matchesHash(entry->getFullPath(), algorithm, expected))
        invalidate(entry);
    return entry;
}

}  // namespace Modpack::Cache
