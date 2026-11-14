// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QCryptographicHash>
#include "net/HttpMetaCache.h"

namespace Modpack::Cache {
enum class Platform { Modrinth, CurseForge, ATLauncher, FTB };

bool matchesHash(const QString& path, QCryptographicHash::Algorithm algorithm, const QByteArray& expected);
void invalidate(const MetaEntryPtr& entry);
MetaEntryPtr resolveEntry(HttpMetaCache& cache, Platform platform, QCryptographicHash::Algorithm algorithm, const QByteArray& expected);
}  // namespace Modpack::Cache
