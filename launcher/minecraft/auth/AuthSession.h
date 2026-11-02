#pragma once

#include <QString>
#include <memory>

#include "LaunchMode.h"

class MinecraftAccount;

struct AuthSession {
    bool MakeOffline(QString offline_playername);
    void MakeDemo(QString name, QString uuid);

    QString serializeUserProperties();

    // combined session ID
    QString session;
    // volatile auth token
    QString access_token;
    // profile name
    QString player_name;
    // profile ID
    QString uuid;
    // 'msa', 'elyby', 'custom' or 'offline', used for account lock
    QString account_type;
    // 'msa' or 'offline', depending on account type
    QString user_type;
    // is ely patch required?
    bool wants_ely_patch = false;
    // is authlib-injector required?
    bool wants_authlib_injector = false;
    // URL for authlib-injector
    QString authlib_injector_auth_url;
    // the actual launch mode for this session
    LaunchMode launchMode;
};

using AuthSessionPtr = std::shared_ptr<AuthSession>;
