// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2026 so5iso4ka <so5iso4ka@icloud.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <utility>

#include "Application.h"
#include "launch/LaunchTask.h"
#include "meta/Index.h"
#include "minecraft/PackProfile.h"
#include "tasks/SequentialTask.h"

#include "ApplyLibraryOverride.h"

ApplyLibraryOverride::ApplyLibraryOverride(LaunchTask* parent, RuntimeContext& ctx, Net::Mode netMode)
    : LaunchStep(parent), m_instance(m_parent->instance()), m_ctx(&ctx), m_netMode(netMode)
{}

ApplyLibraryOverride::~ApplyLibraryOverride() = default;

void ApplyLibraryOverride::startMetaTask(const QString& uid)
{
    Meta::Index* index = APPLICATION->metadataIndex();

    auto versionList = index->get(uid);

    if (versionList->isLoaded()) {
        return onMetaRequestDone(versionList);
    }

    auto task = makeShared<SequentialTask>(QString("Load meta for %1").arg(uid));
    task->addTask(index->loadTask(m_netMode));
    task->addTask(versionList->loadTask(m_netMode));

    m_task = task;

    connect(m_task.get(), &Task::succeeded, this, [this, versionList] { onMetaRequestDone(versionList); });
    connect(m_task.get(), &Task::failed, this, [this, uid](QString reason) {
        emit logLine(tr("Version list for '%1' could not be loaded:\n%2").arg(uid, reason), MessageLevel::Error);
        emitFailed(std::move(reason));
    });

    m_task->start();
}

void ApplyLibraryOverride::startApplyTask(const Meta::Version::Ptr& version)
{
    if (!version->isLoaded()) {
        m_task = APPLICATION->metadataIndex()->loadVersion(version->uid(), version->version(), m_netMode);
        connect(m_task.get(), &Task::succeeded, this, [this, version = version] { apply(version); });
        connect(m_task.get(), &Task::failed, this, [this, version = version->name()](QString reason) {
            emit logLine(tr("Library '%1' could not be loaded:\n%2").arg(version, reason), MessageLevel::Error);
            emitFailed(std::move(reason));
        });
        m_task->start();
    } else {
        apply(version);
    }
}

void ApplyLibraryOverride::apply(const Meta::Version::Ptr& version)
{
    version->data()->applyTo(m_instance->getPackProfile()->getProfile().get(), *m_ctx);
    emitSucceeded();
}