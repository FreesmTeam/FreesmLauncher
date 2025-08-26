// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Freesm Launcher - Minecraft Launcher
 *  Copyright (C) 2025 so5iso4ka <so5iso4ka@icloud.com>
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

#include "Application.h"
#include "discord/DiscordIntegration.h"
#include "launch/LaunchTask.h"

#include "SetDiscordActivity.h"

SetDiscordActivity::SetDiscordActivity(LaunchTask* task, bool playing) : LaunchStep(task), m_playing(playing) {}

void SetDiscordActivity::executeTask()
{
    bool ok{};
    do {
        const auto discord = APPLICATION->discord();
        if (!discord)
            break;

        LaunchTask* launchTask = m_parent;
        if (!launchTask)
            break;

        if (m_playing) {
            discord->instanceStarted(launchTask->instance());
        } else {
            discord->instanceStopped(launchTask->instance());
        }

        ok = true;
    } while (false);

    if (!ok)
        emit logLine("Unable to set Discord activity", MessageLevel::Message);

    // continue even if cannot
    emitSucceeded();
}
