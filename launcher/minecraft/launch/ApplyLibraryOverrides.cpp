#include <algorithm>
#include <memory>

#include "Application.h"
#include "launch/LaunchTask.h"
#include "meta/EplMeta.h"
#include "minecraft/PackProfile.h"

#include "ApplyLibraryOverrides.h"

ApplyLibraryOverrides::ApplyLibraryOverrides(LaunchTask* parent) : LaunchStep(parent), m_instance(m_parent->instance()) {}

void ApplyLibraryOverrides::executeTask()
{
    auto meta = APPLICATION->eplMetadata();
    if (meta->isLoaded())
        return onLibraryOverrideDownloadFinished();

    m_task = meta->loadTask();
    connect(m_task.get(), &Task::succeeded, this, &ApplyLibraryOverrides::onLibraryOverrideDownloadFinished);
    connect(m_task.get(), &Task::failed, this, [this] { emitFailed(tr("Couldn't fetch EPL metadata")); });
    connect(m_task.get(), &Task::aborted, this, [this] { emitFailed(tr("Aborted")); });

    if (!m_task->isRunning()) {
        m_task->start();
    }
}

void ApplyLibraryOverrides::onLibraryOverrideDownloadFinished()
{
    const auto meta = APPLICATION->eplMetadata();
    const auto profile = m_instance->getPackProfile()->getProfile();
    auto& libraries = profile->libraries();

    if (const auto it = std::find_if(libraries.begin(), libraries.end(),
                                     [](const LibraryPtr& lib) { return lib->artifactPrefix() == "com.mojang:authlib"; });
        it != libraries.end()) {
        const auto override = meta->overrideFromVersion((*it)->version());
        if (override.isEmpty())
            emitFailed(tr("No suitable authlib version found"));

        const auto newLibrary = std::make_shared<Library>(override["name"].toString());

        const auto newDownloadInfo = std::make_shared<MojangDownloadInfo>();
        newDownloadInfo->sha1 = override["sha1"].toString();
        newDownloadInfo->url = override["url"].toString();
        newDownloadInfo->size = override["size"].toInt();

        newLibrary->setMojangDownloadInfo(std::make_shared<MojangLibraryDownloadInfo>(newDownloadInfo));

        *it = newLibrary;

        emitSucceeded();
        return;
    }

    emitFailed(tr("Couldn't replace authlib"));
}