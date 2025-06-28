// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ModFolderModel.h"

#include <FileSystem.h>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QIcon>
#include <QMimeData>
#include <QString>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>
#include <algorithm>

#include "minecraft/Component.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "modplatform/ModIndex.h"

ModFolderModel::ModFolderModel(const QDir& dir, BaseInstance* instance, bool is_indexed, bool create_dir, QObject* parent)
    : ResourceFolderModel(QDir(dir), instance, is_indexed, create_dir, parent)
{
    m_column_names = QStringList({ "Enable", "Image", "Name", "Version", "Last Modified", "Provider", "Size", "Side", "Loaders",
                                   "Minecraft Versions", "Release Type", "Requires", "Required by" });
    m_column_names_translated =
        QStringList({ tr("Enable"), tr("Image"), tr("Name"), tr("Version"), tr("Last Modified"), tr("Provider"), tr("Size"), tr("Side"),
                      tr("Loaders"), tr("Minecraft Versions"), tr("Release Type"), tr("Requires "), tr("Required by") });
    m_column_sort_keys = { SortType::ENABLED,      SortType::NAME,     SortType::NAME,       SortType::VERSION, SortType::DATE,
                           SortType::PROVIDER,     SortType::SIZE,     SortType::SIDE,       SortType::LOADERS, SortType::MC_VERSIONS,
                           SortType::RELEASE_TYPE, SortType::REQUIRES, SortType::REQUIRED_BY };
    m_column_resize_modes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,     QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                              QHeaderView::Interactive };
    m_columnsHideable = { false, true, false, true, true, true, true, true, true, true, true, true, true };

    connect(this, &ModFolderModel::parseFinished, this, &ModFolderModel::onParseFinished);
}

QVariant ModFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return m_resources[row]->name();
                case VersionColumn: {
                    switch (at(row).type()) {
                        case ResourceType::FOLDER:
                            return tr("Folder");
                        case ResourceType::SINGLEFILE:
                            return tr("File");
                        default:
                            break;
                    }
                    return at(row).version();
                }
                case DateColumn:
                    return at(row).dateTimeChanged();
                case ProviderColumn: {
                    return at(row).provider();
                }
                case SideColumn: {
                    return at(row).side();
                }
                case LoadersColumn: {
                    return at(row).loaders();
                }
                case McVersionsColumn: {
                    return at(row).mcVersions();
                }
                case ReleaseTypeColumn: {
                    return at(row).releaseType();
                }
                case SizeColumn: {
                    return at(row).sizeStr();
                }
                case RequiredByColumn: {
                    return at(row).requiredByCount();
                }
                case RequiresColumn: {
                    return at(row).requiresCount();
                }
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            if (column == NameColumn) {
                if (at(row).isSymLinkUnder(instDirPath())) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original."
                              "\nCanonical Path: %1")
                               .arg(at(row).fileinfo().canonicalFilePath());
                }
                if (at(row).isMoreThanOneHardLink()) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
                }
            }
            return m_resources[row]->internal_id();
        case Qt::DecorationRole: {
            if (column == NameColumn && (at(row).isSymLinkUnder(instDirPath()) || at(row).isMoreThanOneHardLink()))
                return QIcon::fromTheme("status-yellow");
            if (column == ImageColumn) {
                return at(row).icon({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            return {};
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            return {};
        case Qt::CheckStateRole:
            if (column == ActiveColumn)
                return at(row).enabled() ? Qt::Checked : Qt::Unchecked;
            return QVariant();
        default:
            return QVariant();
    }
}

QVariant ModFolderModel::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                case NameColumn:
                case VersionColumn:
                case DateColumn:
                case ProviderColumn:
                case ImageColumn:
                case SideColumn:
                case LoadersColumn:
                case McVersionsColumn:
                case ReleaseTypeColumn:
                case SizeColumn:
                case RequiredByColumn:
                case RequiresColumn:
                    return columnNames().at(section);
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Is the mod enabled?");
                case NameColumn:
                    return tr("The name of the mod.");
                case VersionColumn:
                    return tr("The version of the mod.");
                case DateColumn:
                    return tr("The date and time this mod was last changed (or added).");
                case ProviderColumn:
                    return tr("The source provider of the mod.");
                case SideColumn:
                    return tr("On what environment the mod is running.");
                case LoadersColumn:
                    return tr("The mod loader.");
                case McVersionsColumn:
                    return tr("The supported minecraft versions.");
                case ReleaseTypeColumn:
                    return tr("The release type.");
                case SizeColumn:
                    return tr("The size of the mod.");
                case RequiredByColumn:
                    return tr("Number of mods for what this is needed.");
                case RequiresColumn:
                    return tr("Number of mods that this requires.");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
    return QVariant();
}

int ModFolderModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : NUM_COLUMNS;
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

void ModFolderModel::onParseSucceeded(int ticket, QString mod_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd())
        return;

    int row = m_resources_index[mod_id];

    auto parse_task = *iter;
    auto cast_task = static_cast<LocalModParseTask*>(parse_task.get());

    Q_ASSERT(cast_task->token() == ticket);

    auto resource = find(mod_id);

    auto result = cast_task->result();
    if (result && resource)
        static_cast<Mod*>(resource.get())->finishResolvingWithDetails(std::move(result->details));

    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}

Mod* findById(QList<Mod*> mods, QString modId)
{
    auto found = std::find_if(mods.begin(), mods.end(), [modId](Mod* m) { return m->mod_id() == modId; });
    return found != mods.end() ? *found : nullptr;
}

void ModFolderModel::onParseFinished()
{
    if (hasPendingParseTasks()) {
        return;
    }
    auto mods = allMods();

    auto findByProjectID = [mods](QVariant modId, ModPlatform::ResourceProvider provider) -> Mod* {
        auto found = std::find_if(mods.begin(), mods.end(), [modId, provider](Mod* m) {
            return m->metadata() && m->metadata()->provider == provider && m->metadata()->project_id == modId;
        });
        return found != mods.end() ? *found : nullptr;
    };
    for (auto mod : mods) {
        auto id = mod->mod_id();
        for (auto dep : mod->dependencies()) {
            auto d = findById(mods, dep);
            if (d) {
                m_requires[id] << d;
                m_requiredBy[d->mod_id()] << mod;
            }
        }
        if (mod->metadata()) {
            for (auto dep : mod->metadata()->dependencies) {
                if (dep.type == ModPlatform::DependencyType::REQUIRED) {
                    auto d = findByProjectID(dep.addonId, mod->metadata()->provider);
                    if (d) {
                        m_requires[id] << d;
                        m_requiredBy[d->mod_id()] << mod;
                    }
                }
            }
        }
    }
    auto removeDuplicates = [](QList<Mod*>& list) {
        std::set<QString> seen;
        auto it = std::remove_if(list.begin(), list.end(), [&seen](Mod* m) {
            auto id = m->mod_id();
            if (seen.count(id) > 0) {
                return true;
            }
            seen.insert(id);
            return false;
        });
        list.erase(it, list.end());
    };
    for (auto key : m_requiredBy.keys()) {
        removeDuplicates(m_requiredBy[key]);
    }
    for (auto key : m_requires.keys()) {
        removeDuplicates(m_requires[key]);
    }
    for (auto mod : mods) {
        auto id = mod->mod_id();
        mod->setRequiredByCount(m_requiredBy[id].count());
        mod->setRequiresCount(m_requires[id].count());
        int row = m_resources_index[mod->internal_id()];
        emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
    }
}

QModelIndexList ModFolderModel::getAffectedMods(const QModelIndexList& indexes, EnableAction action)
{
    if (indexes.isEmpty())
        return {};

    QModelIndexList affectedList = {};
    auto indexedMods = selectedMods(indexes);
    if (action == EnableAction::TOGGLE) {
        if (indexedMods.length() != 1) {
            return {};  // not sure how to handle a bunch of rows that are toggled(not even sure it is posible)
        }
        action = indexedMods.first()->enabled() ? EnableAction::DISABLE : EnableAction::ENABLE;
    }

    std::set<QString> seen;
    bool shouldBeEnabled = action == EnableAction::ENABLE;
    for (auto mod : indexedMods) {
        auto id = mod->mod_id();
        QList<Mod*> mods;
        switch (action) {
            case EnableAction::DISABLE: {
                mods = m_requiredBy[id];
                break;
            }
            case EnableAction::ENABLE: {
                mods = m_requires[id];
                break;
            }
            case EnableAction::TOGGLE:
                break;
        }
        for (auto affected : mods) {
            auto affectedId = affected->mod_id();

            if (findById(indexedMods, affectedId) == nullptr && seen.count(affectedId) == 0) {
                seen.insert(affectedId);
                if (shouldBeEnabled != affected->enabled()) {
                    auto row = m_resources_index[affected->internal_id()];
                    affectedList << index(row, 0);
                }
            }
        }
    }
    // collect the affected mods until all of them are included in the list
    if (!affectedList.isEmpty()) {
        affectedList += getAffectedMods(indexes + affectedList, action);
    }
    return affectedList;
}

bool ModFolderModel::setResourceEnabled(const QModelIndexList& indexes, EnableAction action)
{
    auto affected = getAffectedMods(indexes, action);
    return ResourceFolderModel::setResourceEnabled(indexes + affected, action);
}

QStringList reqToList(QList<Mod*> l)
{
    QStringList req;
    for (auto m : l) {
        req << m->name();
    }
    return req;
}
QStringList ModFolderModel::requiresList(QString id)
{
    return reqToList(m_requires[id]);
}
QStringList ModFolderModel::requiredByList(QString id)
{
    return reqToList(m_requiredBy[id]);
}
