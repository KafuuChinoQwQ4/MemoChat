#pragma once

#include "data.h"

#include <memory>
#include <string>
#include <vector>

class IChatSessionRepository
{
public:
    virtual ~IChatSessionRepository() = default;

    virtual bool GetLegacyToken(int uid, std::string& token) = 0;
    virtual std::string AcquireDuplicateLoginLock(int uid) = 0;
    virtual void ReleaseDuplicateLoginLock(int uid, const std::string& lock_identifier) = 0;
    virtual bool GetUndeliveredPrivateMessages(int uid,
                                               int64_t before_ts,
                                               const std::string& before_msg_id,
                                               int limit,
                                               std::vector<std::shared_ptr<PrivateMessageInfo>>& messages) = 0;
};
