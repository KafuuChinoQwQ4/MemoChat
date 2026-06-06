#pragma once

#include "ports/IChatSessionRepository.h"

class ChatSessionRepository : public IChatSessionRepository
{
public:
    bool GetLegacyToken(int uid, std::string& token) override;
    std::string AcquireDuplicateLoginLock(int uid) override;
    void ReleaseDuplicateLoginLock(int uid, const std::string& lock_identifier) override;
    bool GetUndeliveredPrivateMessages(int uid,
                                       int64_t before_ts,
                                       const std::string& before_msg_id,
                                       int limit,
                                       std::vector<std::shared_ptr<PrivateMessageInfo>>& messages) override;
};
