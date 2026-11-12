// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "MetaCacheSink.h"

namespace Net {
// A cached download succeeds only after a verified copy is available at targetPath.
class CachedFileSink final : public MetaCacheSink {
   public:
    CachedFileSink(HttpMetaCache& cache, MetaEntryPtr entry, QString targetPath, Checksum checksum, bool eternal = false);
    auto hasLocalData() -> bool override;

   protected:
    auto initCache(QNetworkRequest& request) -> Task::State override;
    auto finalizeCache(QNetworkReply& reply) -> Task::State override;

   private:
    enum class CopyResult { Succeeded, InvalidCache, Failed };
    bool validFile(const QString& path) const;
    CopyResult copyToTarget();
    void invalidate();

    MetaEntryPtr m_entry;
    QString m_targetPath;
    Checksum m_checksum;
};
}  // namespace Net
