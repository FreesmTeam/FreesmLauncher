// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
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

#include "BaseInstance.h"

#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>

#include "settings/INISettingsObject.h"
#include "settings/OverrideSetting.h"
#include "settings/Setting.h"

#include "BuildConfig.h"
#include "Commandline.h"
#include "FileSystem.h"

BaseInstance::BaseInstance(SettingsObjectPtr globalSettings, SettingsObjectPtr settings, const QString& rootDir) : QObject()
{
    m_settings = settings;
    m_global_settings = globalSettings;
    m_rootDir = rootDir;

    m_settings->registerSetting("name", "Unnamed Instance");
    m_settings->registerSetting("iconKey", "default");
    m_settings->registerSetting("notes", "");

    m_settings->registerSetting("lastLaunchTime", 0);
    m_settings->registerSetting("totalTimePlayed", 0);
    if (m_settings->get("totalTimePlayed").toLongLong() < 0)
        m_settings->reset("totalTimePlayed");
    m_settings->registerSetting("lastTimePlayed", 0);

    m_settings->registerSetting("linkedInstances", "[]");

    // Game time override
    auto gameTimeOverride = m_settings->registerSetting("OverrideGameTime", false);
    m_settings->registerOverride(globalSettings->getSetting("ShowGameTime"), gameTimeOverride);
    m_settings->registerOverride(globalSettings->getSetting("RecordGameTime"), gameTimeOverride);

    // NOTE: Sometimees InstanceType is already registered, as it was used to identify the type of
    // a locally stored instance
    if (!m_settings->getSetting("InstanceType"))
        m_settings->registerSetting("InstanceType", "");

    // Custom Commands
    auto commandSetting = m_settings->registerSetting({ "OverrideCommands", "OverrideLaunchCmd" }, false);
    m_settings->registerOverride(globalSettings->getSetting("PreLaunchCommand"), commandSetting);
    m_settings->registerOverride(globalSettings->getSetting("WrapperCommand"), commandSetting);
    m_settings->registerOverride(globalSettings->getSetting("PostExitCommand"), commandSetting);

    // Console
    auto consoleSetting = m_settings->registerSetting("OverrideConsole", false);
    m_settings->registerOverride(globalSettings->getSetting("ShowConsole"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("AutoCloseConsole"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("ShowConsoleOnError"), consoleSetting);
    m_settings->registerOverride(globalSettings->getSetting("LogPrePostOutput"), consoleSetting);

    m_settings->registerPassthrough(globalSettings->getSetting("ConsoleMaxLines"), nullptr);
    m_settings->registerPassthrough(globalSettings->getSetting("ConsoleOverflowStop"), nullptr);

    // Managed Packs
    m_settings->registerSetting("ManagedPack", false);
    m_settings->registerSetting("ManagedPackType", "");
    m_settings->registerSetting("ManagedPackID", "");
    m_settings->registerSetting("ManagedPackName", "");
    m_settings->registerSetting("ManagedPackVersionID", "");
    m_settings->registerSetting("ManagedPackVersionName", "");

    m_settings->registerSetting("Profiler", "");
}

QString BaseInstance::getPreLaunchCommand()
{
    return settings()->get("PreLaunchCommand").toString();
}

QString BaseInstance::getWrapperCommand()
{
    return settings()->get("WrapperCommand").toString();
}

QString BaseInstance::getPostExitCommand()
{
    return settings()->get("PostExitCommand").toString();
}

bool BaseInstance::isManagedPack() const
{
    return m_settings->get("ManagedPack").toBool();
}

QString BaseInstance::getManagedPackType() const
{
    return m_settings->get("ManagedPackType").toString();
}

QString BaseInstance::getManagedPackID() const
{
    return m_settings->get("ManagedPackID").toString();
}

QString BaseInstance::getManagedPackName() const
{
    return m_settings->get("ManagedPackName").toString();
}

QString BaseInstance::getManagedPackVersionID() const
{
    return m_settings->get("ManagedPackVersionID").toString();
}

QString BaseInstance::getManagedPackVersionName() const
{
    return m_settings->get("ManagedPackVersionName").toString();
}

void BaseInstance::setManagedPack(const QString& type,
                                  const QString& id,
                                  const QString& name,
                                  const QString& versionId,
                                  const QString& version)
{
    m_settings->set("ManagedPack", true);
    m_settings->set("ManagedPackType", type);
    m_settings->set("ManagedPackID", id);
    m_settings->set("ManagedPackName", name);
    m_settings->set("ManagedPackVersionID", versionId);
    m_settings->set("ManagedPackVersionName", version);
}

void BaseInstance::copyManagedPack(BaseInstance& other)
{
    m_settings->set("ManagedPack", other.isManagedPack());
    m_settings->set("ManagedPackType", other.getManagedPackType());
    m_settings->set("ManagedPackID", other.getManagedPackID());
    m_settings->set("ManagedPackName", other.getManagedPackName());
    m_settings->set("ManagedPackVersionID", other.getManagedPackVersionID());
    m_settings->set("ManagedPackVersionName", other.getManagedPackVersionName());
}

int BaseInstance::getConsoleMaxLines() const
{
    auto lineSetting = m_settings->getSetting("ConsoleMaxLines");
    bool conversionOk = false;
    int maxLines = lineSetting->get().toInt(&conversionOk);
    if (!conversionOk) {
        maxLines = lineSetting->defValue().toInt();
        qWarning() << "ConsoleMaxLines has nonsensical value, defaulting to" << maxLines;
    }
    return maxLines;
}

bool BaseInstance::shouldStopOnConsoleOverflow() const
{
    return m_settings->get("ConsoleOverflowStop").toBool();
}

QStringList BaseInstance::getLinkedInstances() const
{
    return m_settings->get("linkedInstances").toStringList();
}

void BaseInstance::setLinkedInstances(const QStringList& list)
{
    auto linkedInstances = m_settings->get("linkedInstances").toStringList();
    m_settings->set("linkedInstances", list);
}

void BaseInstance::addLinkedInstanceId(const QString& id)
{
    auto linkedInstances = m_settings->get("linkedInstances").toStringList();
    linkedInstances.append(id);
    setLinkedInstances(linkedInstances);
}

bool BaseInstance::removeLinkedInstanceId(const QString& id)
{
    auto linkedInstances = m_settings->get("linkedInstances").toStringList();
    int numRemoved = linkedInstances.removeAll(id);
    setLinkedInstances(linkedInstances);
    return numRemoved > 0;
}

bool BaseInstance::isLinkedToInstanceId(const QString& id) const
{
    auto linkedInstances = m_settings->get("linkedInstances").toStringList();
    return linkedInstances.contains(id);
}

void BaseInstance::iconUpdated(QString key)
{
    if (iconKey() == key) {
        emit propertiesChanged(this);
    }
}

void BaseInstance::invalidate()
{
    changeStatus(Status::Gone);
    qDebug() << "Instance" << id() << "has been invalidated.";
}

void BaseInstance::changeStatus(BaseInstance::Status newStatus)
{
    Status status = currentStatus();
    if (status != newStatus) {
        m_status = newStatus;
        emit statusChanged(status, newStatus);
    }
}

BaseInstance::Status BaseInstance::currentStatus() const
{
    return m_status;
}

QString BaseInstance::id() const
{
    return QFileInfo(instanceRoot()).fileName();
}

bool BaseInstance::isRunning() const
{
    return m_isRunning;
}

void BaseInstance::setRunning(bool running)
{
    if (running == m_isRunning)
        return;

    m_isRunning = running;

    emit runningStatusChanged(running);
}

void BaseInstance::setMinecraftRunning(bool running)
{
    if (!settings()->get("RecordGameTime").toBool()) {
        return;
    }

    if (running) {
        m_timeStarted = QDateTime::currentDateTime();
        setLastLaunch(m_timeStarted.toMSecsSinceEpoch());
    } else {
        QDateTime timeEnded = QDateTime::currentDateTime();

        qint64 current = settings()->get("totalTimePlayed").toLongLong();
        settings()->set("totalTimePlayed", current + m_timeStarted.secsTo(timeEnded));
        settings()->set("lastTimePlayed", m_timeStarted.secsTo(timeEnded));

        emit propertiesChanged(this);
    }
}

int64_t BaseInstance::totalTimePlayed() const
{
    qint64 current = m_settings->get("totalTimePlayed").toLongLong();
    if (m_isRunning) {
        QDateTime timeNow = QDateTime::currentDateTime();
        return current + m_timeStarted.secsTo(timeNow);
    }
    return current;
}

int64_t BaseInstance::lastTimePlayed() const
{
    if (m_isRunning) {
        QDateTime timeNow = QDateTime::currentDateTime();
        return m_timeStarted.secsTo(timeNow);
    }
    return m_settings->get("lastTimePlayed").toLongLong();
}

void BaseInstance::resetTimePlayed()
{
    settings()->reset("totalTimePlayed");
    settings()->reset("lastTimePlayed");
}

QString BaseInstance::instanceType() const
{
    return m_settings->get("InstanceType").toString();
}

QString BaseInstance::instanceRoot() const
{
    return m_rootDir;
}

bool BaseInstance::updateInstanceRoot(QWidget* parent)
{
    QString renamingMode = globalSettings()->get("InstRenamingMode").toString();
    if (renamingMode == "MetadataOnly")
        return false;

    auto oldRoot = instanceRoot();
    auto newRoot = FS::PathCombine(QFileInfo(oldRoot).dir().absolutePath(), name());
    if (oldRoot == newRoot)
        return false;

    // Check for conflict
    if (QDir(newRoot).exists()) {
        QMessageBox::warning(parent, tr("Cannot rename instance"),
                             tr("New instance root (%1) already exists. <br />Only the metadata will be renamed.").arg(newRoot));
        return false;
    }

    // Ask if we should rename
    if (renamingMode == "AskEverytime") {
        QMessageBox messageBox(parent);
        messageBox.setText(tr("Do you want to also rename the instance\'s physical directory?"));
        messageBox.setInformativeText(tr("The following renaming operation will be performed: <br/>"
                                         " - Old instance root: %1<br/>"
                                         " - New instance root: %2")
                                          .arg(oldRoot, newRoot));
        messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        messageBox.setDefaultButton(QMessageBox::Yes);
        messageBox.setIcon(QMessageBox::Question);

        auto checkBox = new QCheckBox(tr("&Remember my choice"), parent);
        messageBox.setCheckBox(checkBox);

        auto res = messageBox.exec();
        if (checkBox->isChecked()) {
            if (res == QMessageBox::Yes)
                globalSettings()->set("InstRenamingMode", "PhysicalDir");
            else
                globalSettings()->set("InstRenamingMode", "MetadataOnly");
        }
        if (res == QMessageBox::No)
            return false;
    }

    // Now we can confirm that a renaming is happening
    auto ret = QFile::rename(oldRoot, newRoot);
    if (!ret) {
        QMessageBox::warning(parent, tr("Cannot rename instance"),
                             tr("An error occurred when performing the following renaming operation: <br/>"
                                " - Old instance root: %1<br/>"
                                " - New instance root: %2<br/>"
                                "Only the metadata is renamed.")
                                 .arg(oldRoot, newRoot));
        return false;
    }
    return true;
}

SettingsObjectPtr BaseInstance::settings()
{
    loadSpecificSettings();

    return m_settings;
}

bool BaseInstance::canLaunch() const
{
    return (!hasVersionBroken() && !isRunning());
}

bool BaseInstance::reloadSettings()
{
    return m_settings->reload();
}

qint64 BaseInstance::lastLaunch() const
{
    return m_settings->get("lastLaunchTime").value<qint64>();
}

void BaseInstance::setLastLaunch(qint64 val)
{
    // FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("lastLaunchTime", val);
    emit propertiesChanged(this);
}

void BaseInstance::setNotes(QString val)
{
    // FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("notes", val);
}

QString BaseInstance::notes() const
{
    return m_settings->get("notes").toString();
}

void BaseInstance::setIconKey(QString val)
{
    // FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("iconKey", val);
    emit propertiesChanged(this);
}

QString BaseInstance::iconKey() const
{
    return m_settings->get("iconKey").toString();
}

void BaseInstance::setName(QString val)
{
    // FIXME: if no change, do not set. setting involves saving a file.
    m_settings->set("name", val);
    emit propertiesChanged(this);
}

QString BaseInstance::name() const
{
    return m_settings->get("name").toString();
}

QString BaseInstance::windowTitle() const
{
    return BuildConfig.LAUNCHER_DISPLAYNAME + ": " + name();
}

// FIXME: why is this here? move it to MinecraftInstance!!!
QStringList BaseInstance::extraArguments()
{
    return Commandline::splitArgs(settings()->get("JvmArgs").toString());
}

shared_qobject_ptr<LaunchTask> BaseInstance::getLaunchTask()
{
    return m_launchProcess;
}

void BaseInstance::updateRuntimeContext()
{
    // NOOP
}

bool BaseInstance::isLegacy()
{
    return traits().contains("legacyLaunch") || traits().contains("alphaLaunch");
}
