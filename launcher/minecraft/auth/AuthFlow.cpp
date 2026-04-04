#include "AuthFlow.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <utility>

#include "minecraft/auth/AccountData.h"
#include "tasks/Task.h"

// MSA
#include "minecraft/auth/msa/steps/EntitlementsStep.h"
#include "minecraft/auth/msa/steps/GetSkinStep.h"
#include "minecraft/auth/msa/steps/LauncherLoginStep.h"
#include "minecraft/auth/msa/steps/MSADeviceCodeStep.h"
#include "minecraft/auth/msa/steps/MSAStep.h"
#include "minecraft/auth/msa/steps/MinecraftProfileStep.h"
#include "minecraft/auth/msa/steps/XboxAuthorizationStep.h"
#include "minecraft/auth/msa/steps/XboxProfileStep.h"
#include "minecraft/auth/msa/steps/XboxUserStep.h"

// Elyby
#include "elyby/steps/ElyDeviceCodeStep.h"
#include "elyby/steps/ElyStep.h"
#include "elyby/steps/MinecraftProfileStepEly.h"

// Custom
#include "custom/steps/CustomAuthStep.h"
#include "custom/steps/CustomGetSkinStep.h"
#include "custom/steps/CustomRefreshStep.h"

AuthFlow::AuthFlow(AccountData* data, Action action, QString password) : Task(), m_data(data)
{
    switch (data->type) {
        case AccountType::MSA: {
            if (action == Action::DeviceCode) {
                auto oauthStep = makeShared<MSADeviceCodeStep>(m_data);
                connect(oauthStep.get(), &MSADeviceCodeStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowserWithExtra);
                connect(this, &Task::aborted, oauthStep.get(), &MSADeviceCodeStep::abort);
                m_steps.append(oauthStep);
            } else {
                auto oauthStep = makeShared<MSAStep>(m_data, action == Action::Refresh);
                connect(oauthStep.get(), &MSAStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowser);
                m_steps.append(oauthStep);
            }
            m_steps.append(makeShared<XboxUserStep>(m_data));
            m_steps.append(makeShared<XboxAuthorizationStep>(m_data, &m_data->xboxApiToken, "http://xboxlive.com", "Xbox"));
            m_steps.append(
                makeShared<XboxAuthorizationStep>(m_data, &m_data->mojangservicesToken, "rp://api.minecraftservices.com/", "Mojang"));
            m_steps.append(makeShared<LauncherLoginStep>(m_data));
            m_steps.append(makeShared<XboxProfileStep>(m_data));
            m_steps.append(makeShared<EntitlementsStep>(m_data));
            m_steps.append(makeShared<MinecraftProfileStep>(m_data));
            m_steps.append(makeShared<GetSkinStep>(m_data));
        } break;
        case AccountType::Elyby: {
            if (action == Action::DeviceCode) {
                auto oauthStep = makeShared<ElyDeviceCodeStep>(m_data);
                connect(oauthStep.get(), &ElyDeviceCodeStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowserWithExtra);
                connect(this, &Task::aborted, oauthStep.get(), &ElyDeviceCodeStep::abort);
                m_steps.append(oauthStep);
            } else {
                auto oauthStep = makeShared<ElyStep>(m_data, action == Action::Refresh);
                connect(oauthStep.get(), &ElyStep::authorizeWithBrowser, this, &AuthFlow::authorizeWithBrowser);
                m_steps.append(oauthStep);
            }
            m_steps.append(makeShared<MinecraftProfileStepEly>(m_data));
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
            setStatus(m_currentStep ? m_currentStep->describe() : tr("Working..."));
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
    emitAborted();
    if (m_currentStep)
        m_currentStep->abort();
    return true;
}
