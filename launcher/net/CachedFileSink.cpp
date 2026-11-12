// SPDX-License-Identifier: GPL-3.0-only
#include "CachedFileSink.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <utility>

#include "FileSystem.h"

namespace Net {
CachedFileSink::CachedFileSink(HttpMetaCache& cache, MetaEntryPtr entry, QString targetPath, Checksum checksum, bool eternal)
    : MetaCacheSink(entry, new ChecksumValidator(QCryptographicHash::Md5), eternal, &cache),
      m_entry(std::move(entry)),
      m_targetPath(std::move(targetPath)),
      m_checksum(std::move(checksum))
{
    addValidator(new ChecksumValidator(m_checksum.algorithm, m_checksum.value));
}

bool CachedFileSink::validFile(const QString& path) const
{
    QFile file(path);
    QCryptographicHash hash(m_checksum.algorithm);
    return QFileInfo(path).isFile() && file.open(QIODevice::ReadOnly) && hash.addData(&file) && hash.result() == m_checksum.value;
}

void CachedFileSink::invalidate()
{
    m_entry->setStale(true);
    m_entry->setETag({});
    m_entry->setRemoteChangedTimestamp({});
}

CachedFileSink::CopyResult CachedFileSink::copyToTarget()
{
    QFile input(m_filename);
    if (!input.open(QIODevice::ReadOnly))
        return CopyResult::InvalidCache;

    if (!FS::ensureFilePathExists(m_targetPath)) {
        m_fail_reason = QObject::tr("Could not create the parent directory for %1.").arg(m_targetPath);
        return CopyResult::Failed;
    }
    QSaveFile output(m_targetPath);
    output.setDirectWriteFallback(false);
    if (!output.open(QIODevice::WriteOnly)) {
        m_fail_reason = QObject::tr("Could not write %1: %2").arg(m_targetPath, output.errorString());
        return CopyResult::Failed;
    }
    QCryptographicHash hash(m_checksum.algorithm);
    QByteArray buffer(64 * 1024, '\0');
    for (;;) {
        const auto size = input.read(buffer.data(), buffer.size());
        if (size < 0)
            return CopyResult::InvalidCache;
        if (size == 0)
            break;
        hash.addData(QByteArrayView(buffer.constData(), size));
        if (output.write(buffer.constData(), size) != size) {
            m_fail_reason = QObject::tr("Could not write %1: %2").arg(m_targetPath, output.errorString());
            return CopyResult::Failed;
        }
    }
    if (hash.result() != m_checksum.value)
        return CopyResult::InvalidCache;
    if (!output.commit()) {
        m_fail_reason = QObject::tr("Could not commit %1: %2").arg(m_targetPath, output.errorString());
        return CopyResult::Failed;
    }
    return CopyResult::Succeeded;
}

Task::State CachedFileSink::initCache(QNetworkRequest& request)
{
    if (m_checksum.value.isEmpty() || m_checksum.value.size() != QCryptographicHash::hashLength(m_checksum.algorithm)) {
        m_fail_reason = QObject::tr("Invalid expected checksum for %1.").arg(m_targetPath);
        return Task::State::Failed;
    }
    if (!m_entry->isStale()) {
        switch (copyToTarget()) {
            case CopyResult::Succeeded:
                return Task::State::Succeeded;
            case CopyResult::Failed:
                return Task::State::Failed;
            case CopyResult::InvalidCache:
                invalidate();
                break;
        }
    } else if ((!m_entry->getETag().isEmpty() || !m_entry->getRemoteChangedTimestamp().isEmpty()) && !validFile(m_filename)) {
        // A 304 must never preserve a corrupt file just because its HTTP metadata is still present.
        invalidate();
    }
    return MetaCacheSink::initCache(request);
}

Task::State CachedFileSink::finalizeCache(QNetworkReply& reply)
{
    const auto result = MetaCacheSink::finalizeCache(reply);
    if (result != Task::State::Succeeded)
        return result;
    const auto copied = copyToTarget();
    if (copied == CopyResult::Succeeded)
        return Task::State::Succeeded;
    if (copied == CopyResult::InvalidCache) {
        invalidate();
        m_fail_reason = QObject::tr("Cached file failed checksum validation for %1.").arg(m_targetPath);
    }
    return Task::State::Failed;
}

bool CachedFileSink::hasLocalData()
{
    // AcceptLocalFiles may only bypass a network failure if the actual target is ready.
    return validFile(m_targetPath);
}
}  // namespace Net
