#include "ChatSessionRepository.hpp"

#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"

import memochat.chat.session_repository_algorithms;

namespace
{
namespace session_repository_modules = memochat::chat::persistence::session_repository::modules;

std::string DuplicateLoginLockKey(int uid)
{
    return std::string(LOCK_PREFIX) + std::to_string(uid);
}
} // namespace

std::string ChatSessionRepository::AcquireDuplicateLoginLock(int uid)
{
    if (!session_repository_modules::ShouldAcquireDuplicateLoginLock(uid))
    {
        return std::string();
    }
    return RedisMgr::GetInstance()->acquireLock(DuplicateLoginLockKey(uid), LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
}

void ChatSessionRepository::ReleaseDuplicateLoginLock(int uid, const std::string& lock_identifier)
{
    if (!session_repository_modules::ShouldReleaseDuplicateLoginLock(uid, lock_identifier.empty()))
    {
        return;
    }
    RedisMgr::GetInstance()->releaseLock(DuplicateLoginLockKey(uid), lock_identifier);
}

bool ChatSessionRepository::GetUndeliveredPrivateMessages(int uid,
                                                          int64_t before_ts,
                                                          const std::string& before_msg_id,
                                                          int limit,
                                                          std::vector<std::shared_ptr<PrivateMessageInfo>>& messages)
{
    if (!session_repository_modules::ShouldQueryUndeliveredPrivateMessages(uid, limit))
    {
        return false;
    }
    return PostgresMgr::GetInstance()->GetUndeliveredPrivateMessages(uid, before_ts, before_msg_id, limit, messages);
}
