#pragma once
#include "minecraft/VersionFilterData.h"
#include "net/NetJob.h"
#include "tasks/Task.h"

class MinecraftInstance;

class LegacyFMLLibrariesTask : public Task {
    Q_OBJECT
   public:
    LegacyFMLLibrariesTask(MinecraftInstance* inst);
    virtual ~LegacyFMLLibrariesTask() = default;

    void executeTask() override;

    bool canAbort() const override;
    bool downloadFailed() const { return m_downloadFailed; }

   private slots:
    void fmllibsFinished();
    void fmllibsFailed(QString reason);

   public slots:
    bool abort() override;

   private:
    static QString baseUrl();

   private:
    MinecraftInstance* m_inst;
    NetJob::Ptr downloadJob;
    QList<FMLlib> fmlLibsToProcess;
    bool m_downloadFailed = false;
};
