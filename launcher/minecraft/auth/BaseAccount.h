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

#include <QObject>

#include <settings/SettingsObject.h>
#include "AccountData.h"
#include "AuthFlow.h"
#include "AuthSession.h"
#include "Usable.h"

class BaseAccount;

using BaseAccountPtr = shared_qobject_ptr<BaseAccount>;
Q_DECLARE_METATYPE(BaseAccountPtr)

struct AccountProfile {
    QString id;
    QString name;
    bool legacy;
};

class BaseAccount : public QObject, public Usable {
    Q_OBJECT

   public:
    //! Do not copy accounts. ever.
    explicit BaseAccount(const BaseAccount& other, QObject* parent) = delete;

    explicit BaseAccount(QObject* parent = nullptr);

   public: /* static methods */
    static BaseAccountPtr loadFromJsonV3(const QJsonObject& json);

    static QUuid uuidFromUsername(QString username);

   public: /*methods*/
    //! Saves an account to a JSON object and returns it.
    QJsonObject saveToJson() const { return data.saveState(); }

    virtual shared_qobject_ptr<AuthFlow> refresh();

    shared_qobject_ptr<AuthFlow> currentTask() { return m_currentTask; }

    AccountData* accountData() { return &data; }

   public: /* queries */
    QString internalId() const
    {
        qDebug() << data.internalId;
        return data.internalId;
    }

    QString accountDisplayString() const { return data.accountDisplayString(); }

    QString accessToken() const { return data.accessToken(); }

    QString profileId() const { return data.profileId(); }

    QString profileName() const { return data.profileName(); }

    bool isActive() const { return !m_currentTask.isNull(); }

    [[nodiscard]] AccountType accountType() const noexcept { return data.type; }

    bool ownsMinecraft() const { return true; }

    bool hasProfile() const { return data.profileId().size() != 0; }

    QString typeString() const
    {
        switch (data.type) {
            case AccountType::MSA: {
                return "msa";
            } break;
            case AccountType::Elyby: {
                return "elyby";
            } break;
            case AccountType::Offline: {
                return "offline";
            } break;
            default: {
                return "unknown";
            }
        }
    }

    QPixmap getFace() const;

    AccountState accountState() const { return data.accountState; }

    bool shouldRefresh() const;

    void fillSession(AuthSessionPtr session, SettingsObjectPtr instanceSettings);

    QString lastError() const { return data.lastError(); }

   signals:
    /**
     * This signal is emitted when the account changes
     */
    void changed();

    void activityChanged(bool active);

   protected: /* methods */
    void incrementUses() override;
    void decrementUses() override;

   protected: /* data */
    AccountData data;

    // current task we are executing here
    shared_qobject_ptr<AuthFlow> m_currentTask;

   protected slots:
    void authSucceeded();
    void authFailed(QString reason);
};