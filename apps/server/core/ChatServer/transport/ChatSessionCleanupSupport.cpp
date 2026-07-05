#include "ChatSessionCleanupSupport.hpp"

#include "IChatSessionHost.hpp"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "UserMgr.hpp"
#include "const.hpp"

namespace memochat::chatserver
{

void CleanupExceptionSession(const std::shared_ptr<IChatSession>& session, IChatSessionHost* host)
{
    if (!session || session->userId() <= 0)
    {
        return;
    }

    const auto uid = session->userId();
    const auto session_id = session->sessionId();
    const auto uid_str = std::to_string(uid);
    const auto lock_key = LOCK_PREFIX + uid_str;
    const auto identifier = RedisMgr::GetInstance()->acquireLock(lock_key, LOCK_TIME_OUT, ACQUIRE_TIME_OUT);
    Defer defer(
        [identifier, lock_key, host, uid, session_id]()
        {
            if (host != nullptr)
            {
                host->ClearSession(session_id);
            }
            else
            {
                UserMgr::GetInstance()->RmvUserSession(uid, session_id);
            }
            RedisMgr::GetInstance()->releaseLock(lock_key, identifier);
        });

    if (identifier.empty())
    {
        return;
    }

    std::string redis_session_id;
    if (!RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id))
    {
        return;
    }

    if (redis_session_id != session_id)
    {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    RedisMgr::GetInstance()->SRem(std::string(SERVER_ONLINE_USERS_PREFIX) + cfg["SelfServer"]["Name"], uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
}

} // namespace memochat::chatserver
