#include "ChatSessionRepository.h"

#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"

bool ChatSessionRepository::GetLegacyToken(int uid, std::string& token)
{
    if (uid <= 0)
    {
        return false;
    }
    return RedisMgr::GetInstance()->Get(std::string(USERTOKENPREFIX) + std::to_string(uid), token);
}

std::string ChatSessionRepository::AcquireDuplicateLoginLock(int uid)
{
    if (uid <= 0)
    {
        return std::string();
    }
    return RedisMgr::GetInstance()->acquireLock(std::string(LOCK_PREFIX) + std::to_string(uid),
                                                LOCK_TIME_OUT,
                                                ACQUIRE_TIME_OUT);
}

void ChatSessionRepository::ReleaseDuplicateLoginLock(int uid, const std::string& lock_identifier)
{
    if (uid <= 0 || lock_identifier.empty())
    {
        return;
    }
    RedisMgr::GetInstance()->releaseLock(std::string(LOCK_PREFIX) + std::to_string(uid), lock_identifier);
}

bool ChatSessionRepository::GetUndeliveredPrivateMessages(int uid,
                                                          int64_t before_ts,
                                                          const std::string& before_msg_id,
                                                          int limit,
                                                          std::vector<std::shared_ptr<PrivateMessageInfo>>& messages)
{
    return PostgresMgr::GetInstance()->GetUndeliveredPrivateMessages(uid, before_ts, before_msg_id, limit, messages);
}
