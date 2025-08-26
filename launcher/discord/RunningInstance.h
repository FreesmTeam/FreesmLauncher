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

#pragma once

class BaseInstance;

#include <QDateTime>
#include <utility>

struct WeakRunningInstance {
    WeakRunningInstance() = default;

    explicit WeakRunningInstance(std::weak_ptr<BaseInstance> instance)
        : startedAt(QDateTime::currentDateTimeUtc()), instance(std::move(instance))
    {}

    auto expired() const { return instance.expired(); }

    auto lock() const { return instance.lock(); }

    QDateTime startedAt;
    std::weak_ptr<BaseInstance> instance;
};

struct RunningInstance {
    explicit RunningInstance(std::shared_ptr<BaseInstance> instance)
        : startedAt(QDateTime::currentDateTimeUtc()), instance(std::move(instance))
    {}

    explicit RunningInstance(const WeakRunningInstance& instance) : startedAt(instance.startedAt), instance(instance.lock()) {}

    auto ptr() const { return instance; }

    QDateTime startedAt;
    std::shared_ptr<BaseInstance> instance;
};
