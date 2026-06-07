#include "ClaimAccount.h"
#include <launch/LaunchTask.h>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

ClaimAccount::ClaimAccount(LaunchTask* parent, AuthSessionPtr session) : LaunchStep(parent)
{
    if (session->launchMode == LaunchMode::Normal) {
        auto accounts = APPLICATION->accounts();
        auto idx = accounts->findAccountById(AccountIdentifier{ session->uuid, session->user_type, session->authlib_injector_auth_url });
        if (idx.found()) {
            m_account = accounts->at(idx.index);
        } else {
            m_account = nullptr;
        }
    }
}

void ClaimAccount::executeTask()
{
    if (m_account) {
        lock.reset(new UseLock(m_account.get()));
    }
    emitSucceeded();
}

void ClaimAccount::finalize()
{
    lock.reset();
}
