// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "net/NetJob.h"

namespace Flame {

// Interrupt downloads, but wait for their sinks to close before reporting a
// terminal state, unlike NetJob::abort() which can finish before requests unwind.
class ContentDownloadJob final : public NetJob {
   public:
    using NetJob::NetJob;

    bool abort() override
    {
        m_stopping = true;
        m_queue.clear();
        setAskRetry(false);
        QMetaObject::invokeMethod(
            this,
            [this] {
                const auto active = m_doing.values();
                for (const auto& task : active)
                    task->abort();
                executeNextSubTask();
            },
            Qt::QueuedConnection);
        return true;
    }

   protected:
    void executeNextSubTask() override
    {
        if (!m_stopping) {
            NetJob::executeNextSubTask();
        } else if (isRunning() && m_doing.isEmpty()) {
            emitAborted();
        }
    }

   private:
    bool m_stopping = false;
};

}  // namespace Flame
