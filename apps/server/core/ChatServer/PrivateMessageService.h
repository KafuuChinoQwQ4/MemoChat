#pragma once

#include <memory>
#include <string>

class CSession;
class LogicSystem;

class PrivateMessageService {
public:
    explicit PrivateMessageService(LogicSystem& logic);

    void HandleTextChatMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleForwardPrivateMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandlePrivateReadAck(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleEditPrivateMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleRevokePrivateMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandlePrivateHistory(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);

private:
    LogicSystem& _logic;
};
