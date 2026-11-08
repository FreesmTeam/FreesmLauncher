#include "AuthFlow.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <utility>

#include "Application.h"
#include "minecraft/auth/AccountData.h"
#include "tasks/Task.h"

// Shared
#include "steps/GetSkinStep.h"
#include "steps/MinecraftProfileStep.h"
#include "steps/OAuth2AuthCodeStep.h"
#include "steps/OAuth2DeviceAuthStep.h"

// MSA
#include "steps/EntitlementsStep.h"
#include "steps/LauncherLoginStep.h"
#include "steps/XboxAuthorizationStep.h"
#include "steps/XboxUserStep.h"

// Elyby
#include "steps/ElyYggdrasilTokenStep.h"

// Custom
#include "steps/CustomGetSkinStep.h"
#include "steps/CustomAuthStep.h"
#include "steps/CustomRefreshStep.h"

AuthFlow::AuthFlow(AccountData* data, Action action, QString password) : Task(), m_data(data)
{
    const auto type = data->type;
    const bool usesOAuth2 = type == AccountType::MSA || type == AccountType::Elyby;
    if (usesOAuth2) {
        OAuth2ServerInfo info;
        if (type == AccountType::MSA) {
            info = OAuth2ServerInfo{
                .displayName = "Microsoft",
                .clientId = APPLICATION->getMSAClientID(),
                .scopes = "XboxLive.SignIn XboxLive.offline_access",
                .authorizationUrl = QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"),
                .accessTokenUrl = QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token"),
                .deviceCodeUrl = QUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode"),
            };
        } else if (type == AccountType::Elyby) {
            info = OAuth2ServerInfo{
                .displayName = "Ely.by",
                .clientId = APPLICATION->getElybyClientID(),
                .scopes = "account_info offline_access minecraft_server_session",
                .authorizationUrl = QUrl("https://account.ely.by/oauth2/v1"),
                .accessTokenUrl = QUrl("https://account.ely.by/api/oauth2/v1/token"),
                .deviceCodeUrl = QUrl("https://account.ely.by/api/oauth2/v1/devicecode"),
            };
        }

        if (action == Action::DeviceCode) {
            auto oauthStep = makeShared<OAuth2DeviceAuthStep>(m_data, info);
            connect(oauthStep.get(), &OAuth2DeviceAuthStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowserWithExtra);
            connect(this, &Task::aborted, oauthStep.get(), &OAuth2DeviceAuthStep::abort);
            m_steps.append(oauthStep);
        } else {
            auto oauthStep = makeShared<OAuth2AuthCodeStep>(m_data, info, action == Action::Refresh);
            connect(oauthStep.get(), &OAuth2AuthCodeStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowser);
            m_steps.append(oauthStep);
        }
    }

    switch (type) {
        case AccountType::MSA: {
            const QUrl profileUrl("https://api.minecraftservices.com/minecraft/profile");

            m_steps.append(makeShared<XboxUserStep>(m_data));
            m_steps.append(
                makeShared<XboxAuthorizationStep>(m_data, &m_data->mojangservicesToken, "rp://api.minecraftservices.com/", "Mojang"));
            m_steps.append(makeShared<LauncherLoginStep>(m_data));
            m_steps.append(makeShared<EntitlementsStep>(m_data));
            m_steps.append(makeShared<MinecraftProfileStep>(m_data, profileUrl));
            m_steps.append(makeShared<GetSkinStep>(m_data));
        } break;
        case AccountType::Elyby: {
            const QUrl profileUrl("https://account.ely.by/api/mojang/services/minecraft/profile");

            m_steps.append(makeShared<ElyYggdrasilTokenStep>(m_data));
            m_steps.append(makeShared<MinecraftProfileStep>(m_data, profileUrl));
            m_steps.append(makeShared<GetSkinStep>(m_data));
        } break;
        case AccountType::Custom: {
            if (action == Action::Login) {
                m_steps.append(makeShared<CustomAuthStep>(m_data, std::move(password)));
                m_steps.append(makeShared<CustomRefreshStep>(m_data));
            } else {
                m_steps.append(makeShared<CustomRefreshStep>(m_data));
            }
            m_steps.append(makeShared<CustomGetSkinStep>(m_data));
        } break;
        default:
            break;
    }

    changeState(AccountTaskState::STATE_CREATED);
}

void AuthFlow::succeed()
{
    m_data->validity_ = Validity::Certain;
    changeState(AccountTaskState::STATE_SUCCEEDED, tr("Finished all authentication steps"));
}

void AuthFlow::executeTask()
{
    changeState(AccountTaskState::STATE_WORKING, tr("Initializing"));
    nextStep();
}

void AuthFlow::nextStep()
{
    if (!Task::isRunning()) {
        return;
    }
    if (m_steps.size() == 0) {
        // we got to the end without an incident... assume this is all.
        m_currentStep.reset();
        succeed();
        return;
    }
    m_currentStep = m_steps.front();
    qDebug() << "AuthFlow:" << m_currentStep->describe();
    setStatus(m_currentStep->describe());
    m_steps.pop_front();
    connect(m_currentStep.get(), &AuthStep::finished, this, &AuthFlow::stepFinished);

    m_currentStep->perform();
}

void AuthFlow::stepFinished(AccountTaskState resultingState, QString message)
{
    if (changeState(resultingState, message))
        nextStep();
}

bool AuthFlow::changeState(AccountTaskState newState, QString reason)
{
    m_taskState = newState;
    setDetails(reason);
    switch (newState) {
        case AccountTaskState::STATE_CREATED: {
            setStatus(tr("Waiting..."));
            m_data->errorString.clear();
            return true;
        }
        case AccountTaskState::STATE_WORKING: {
            if (!m_currentStep) {
                setStatus(tr("Preparing to log in..."));
            }
            m_data->accountState = AccountState::Working;
            return true;
        }
        case AccountTaskState::STATE_SUCCEEDED: {
            setStatus(tr("Authentication task succeeded."));
            m_data->accountState = AccountState::Online;
            emitSucceeded();
            return false;
        }
        case AccountTaskState::STATE_OFFLINE: {
            setStatus(tr("Failed to contact the authentication server."));
            m_data->errorString = reason;
            m_data->accountState = AccountState::Offline;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_DISABLED: {
            setStatus(tr("Client ID has changed. New session needs to be created."));
            m_data->errorString = reason;
            m_data->accountState = AccountState::Disabled;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_SOFT: {
            setStatus(tr("Encountered an error during authentication."));
            m_data->errorString = reason;
            m_data->accountState = AccountState::Errored;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_HARD: {
            setStatus(tr("Failed to authenticate. The session has expired."));
            m_data->errorString = reason;
            m_data->accountState = AccountState::Expired;
            emitFailed(reason);
            return false;
        }
        case AccountTaskState::STATE_FAILED_GONE: {
            setStatus(tr("Failed to authenticate. The account no longer exists."));
            m_data->errorString = reason;
            m_data->accountState = AccountState::Gone;
            emitFailed(reason);
            return false;
        }
        default: {
            setStatus(tr("..."));
            QString error = tr("Unknown account task state: %1").arg(int(newState));
            m_data->accountState = AccountState::Errored;
            emitFailed(error);
            return false;
        }
    }
}
bool AuthFlow::abort()
{
    if (m_currentStep)
        m_currentStep->abort();
    emitAborted();
    return true;
}
