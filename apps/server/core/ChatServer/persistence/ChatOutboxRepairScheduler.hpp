#pragma once

#include "ports/IOutboxRepairScheduler.hpp"

class ChatOutboxRepairScheduler final : public IOutboxRepairScheduler
{
public:
    bool ExpediteOutboxRepair(int64_t outbox_id) override;
};
