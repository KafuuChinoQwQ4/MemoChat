#pragma once

#include <memory>
#include <string>

class CSession;
class LogicSystem;

class ChatSessionService {
public:
    explicit ChatSessionService(LogicSystem& logic);

    void HandleLogin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleRelationBootstrap(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleHeartbeat(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);

private:
    void PushOfflineMessages(const std::shared_ptr<CSession>& session, int uid);
    LogicSystem& _logic;
};
