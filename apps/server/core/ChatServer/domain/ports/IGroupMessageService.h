#pragma once

#include "ports/IMessageCommandService.h"

#include "json/GlazeCompat.h"

#include <memory>
#include <string>

class CSession;

class IGroupMessageService : public IGroupMessageCommandService
{
public:
    virtual ~IGroupMessageService() = default;

    virtual void BuildGroupListJson(int uid, memochat::json::JsonValue& out) = 0;
    virtual void
    HandleCreateGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGetGroupList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleInviteGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleApplyJoinGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleReviewGroupApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGroupChatMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGroupHistory(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleEditGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleRevokeGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleForwardGroupMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGroupReadAck(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>& session,
                                               short msg_id,
                                               const std::string& msg_data) = 0;
    virtual void
    HandleUpdateGroupIcon(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleSetGroupAdmin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleMuteGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleKickGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleQuitGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleDissolveGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
};
