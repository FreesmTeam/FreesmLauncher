#pragma once

#include "MinecraftProfileStep.h"
#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"

class MinecraftProfileStepEly : public MinecraftProfileStep {
    Q_OBJECT

   public:
    explicit MinecraftProfileStepEly(AccountData* data);
    ~MinecraftProfileStepEly() override = default;

    void perform() override;
};
