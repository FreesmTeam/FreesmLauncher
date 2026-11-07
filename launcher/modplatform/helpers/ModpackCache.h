// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QCryptographicHash>
#include "net/HttpMetaCache.h"

namespace Modpack::Cache {
enum class Platform { Modrinth, CurseForge, ATLauncher, FTB, Technic };

bool matchesHash(const QString& path, QCryptographicHash::Algorithm algorithm, const QByteArray& expected);
void invalidate(const MetaEntryPtr& entry);
MetaEntryPtr resolveEntry(HttpMetaCache& cache, Platform platform, QCryptographicHash::Algorithm algorithm, const QByteArray& expected);
bool copyVerified(const QString& source,
                  const QString& destination,
                  QCryptographicHash::Algorithm algorithm,
                  const QByteArray& expected,
                  QString* error);
bool storeFile(HttpMetaCache& cache,
               const MetaEntryPtr& entry,
               const QString& source,
               QCryptographicHash::Algorithm algorithm,
               const QByteArray& expected,
               QString* error);
}  // namespace Modpack::Cache
