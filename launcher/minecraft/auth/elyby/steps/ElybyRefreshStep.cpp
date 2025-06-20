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

#include "ElybyRefreshStep.h"

ElybyRefreshStep::ElybyRefreshStep(AccountData* data) : CustomRefreshStep(data) {}

void ElybyRefreshStep::setSkin()
{
    m_data->minecraftProfile.skin.url = "http://skinsystem.ely.by/skins/" + m_data->minecraftProfile.name + ".png";
}

void ElybyRefreshStep::onRequestDone()
{
    if (!parseResponse()) {
        emit finished(AccountTaskState::STATE_OFFLINE,
                      tr("Failed to refresh authorization for %1 account: %2").arg(authType(), m_request->errorString()));
        return;
    }
    setSkin();
    emit finished(AccountTaskState::STATE_WORKING, tr("Got authorization for %1 account").arg(authType()));
}
