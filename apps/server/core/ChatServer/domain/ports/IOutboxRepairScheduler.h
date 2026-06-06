#pragma once

#include <cstdint>

class IOutboxRepairScheduler
{
public:
    virtual ~IOutboxRepairScheduler() = default;

    virtual bool ExpediteOutboxRepair(int64_t outbox_id) = 0;
};
