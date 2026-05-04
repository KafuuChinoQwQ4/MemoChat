#pragma once

#include "json/GlazeCompat.h"
#include <memory>
#include <string>

class CSession;
class LogicSystem;
class GroupMessageService {
public:
    explicit GroupMessageService(LogicSystem& logic);

    void BuildGroupListJson(int uid, memochat::json::JsonValue& out);
    void HandleCreateGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleGetGroupList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleInviteGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleApplyJoinGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleReviewGroupApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleGroupChatMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleGroupHistory(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleEditGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleRevokeGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleForwardGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleGroupReadAck(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleUpdateGroupIcon(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleSetGroupAdmin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleMuteGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleKickGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleQuitGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleDissolveGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);

private:
    LogicSystem& _logic;
};
