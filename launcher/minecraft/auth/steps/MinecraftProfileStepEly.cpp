#include "MinecraftProfileStepEly.h"

#include "Application.h"
#include "minecraft/auth/Parsers.h"
#include "net/RawHeaderProxy.h"

MinecraftProfileStepEly::MinecraftProfileStepEly(AccountData* data) : MinecraftProfileStep(data) {}

void MinecraftProfileStepEly::perform()
{
    m_data->yggdrasilToken = m_data->msaToken;

    QUrl url("https://account.ely.by/api/mojang/services/minecraft/profile");
    auto headers = QList<Net::HeaderPair>{ { "Content-Type", "application/json" },
                                           { "Accept", "application/json" },
                                           { "Authorization", QString("Bearer %1").arg(m_data->yggdrasilToken.token).toUtf8() } };

    auto [request, response] = Net::Download::makeByteArray(url);
    m_request = std::move(request);
    m_request->addHeaderProxy(std::make_unique<Net::RawHeaderProxy>(headers));

    m_task.reset(new NetJob("MinecraftProfileStepEly", APPLICATION->network()));
    m_task->setAskRetry(false);
    m_task->addNetAction(m_request);

    connect(m_task.get(), &Task::finished, this, [this, response] { onRequestDone(response); });

    m_task->start();
}