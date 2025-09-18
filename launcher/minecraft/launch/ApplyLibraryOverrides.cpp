#include "ApplyLibraryOverrides.h"
#include "Application.h"
#include "BuildConfig.h"
#include "launch/LaunchTask.h"
#include "minecraft/PackProfile.h"
#include "net/Download.h"
#include "net/NetJob.h"

ApplyLibraryOverrides::ApplyLibraryOverrides(LaunchTask* parent, AuthSessionPtr session)
    : LaunchStep(parent), m_session(session), m_instance(m_parent->instance())
{}

void ApplyLibraryOverrides::executeTask()
{
    downloadLibraryOverrideList();
}

void ApplyLibraryOverrides::downloadLibraryOverrideList()
{
    const auto libraryOverrideListUrl = QUrl(m_isFirstDownloadTry ? BuildConfig.EPL_META_URL : BuildConfig.EPL_META_FALLBACK_URL);
    m_response = std::make_shared<QByteArray>();
    m_request = Net::Download::makeByteArray(libraryOverrideListUrl, m_response);

    m_task.reset(new NetJob("Fetch EPL metadata", APPLICATION->network()));
    m_task->addNetAction(m_request);
    m_task->setAskRetry(false);

    connect(m_task.get(), &NetJob::finished, this, &ApplyLibraryOverrides::onLibraryOverrideDownloadFinished);
    connect(m_task.get(), &NetJob::aborted, this, [this] { emitFailed(tr("Aborted")); });

    m_task->start();
}

void ApplyLibraryOverrides::onLibraryOverrideDownloadFinished()
{
    if (m_request->error() != QNetworkReply::NoError) {
        if (m_isFirstDownloadTry) {
            m_isFirstDownloadTry = false;
            return downloadLibraryOverrideList();
        }
        emitFailed("Failed to download EPL metadata.");
        return;
    }

    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(*m_response, &jsonError);
    if (jsonError.error) {
        emitFailed("Failed to parse EPL metadata.");
        return;
    }

    const auto root = doc.object();
    const auto overrides = root.value("overrides").toObject();

    auto& libraries = m_instance->getPackProfile()->getProfile()->libraries();
    for (int i = libraries.size() - 1; i >= 0; --i) {
        const auto library = libraries.at(i);
        const QString& libraryArtifact = library->artifactPrefix();
        const bool isAuthlib = libraryArtifact == "com.mojang:authlib";
        if (isAuthlib && !m_session->wants_ely_patch) {
            continue;
        }

        const QJsonValue artifact = overrides.value(libraryArtifact);
        if (!artifact.isObject()) {
            continue;
        }

        const QJsonValue version = artifact.toObject().value(library->version());
        if (!version.isObject()) {
            continue;
        }

        const QJsonObject override = version.toObject();
        auto newName = override.value("name").toString();

        LibraryPtr newLibrary(new Library(newName));
        const auto newDownloadInfo = std::make_shared<MojangDownloadInfo>();
        newDownloadInfo->sha1 = override.value("sha1").toString();
        newDownloadInfo->url = override.value("url").toString();
        newDownloadInfo->size = override.value("size").toInt();

        const auto newLibraryDownloadInfo = std::make_shared<MojangLibraryDownloadInfo>(newDownloadInfo);
        newLibrary->setMojangDownloadInfo(newLibraryDownloadInfo);

        libraries.removeAt(i);
        libraries.insert(i, newLibrary);
    }

    emitSucceeded();
}