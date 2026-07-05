#include "ChatOutboxRepairScheduler.hpp"

#include "PostgresMgr.hpp"

bool ChatOutboxRepairScheduler::ExpediteOutboxRepair(int64_t outbox_id)
{
    return PostgresMgr::GetInstance()->ExpediteChatOutboxEventRetry(outbox_id);
}
