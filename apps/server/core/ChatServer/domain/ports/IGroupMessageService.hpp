#pragma once

#include "ports/IMessageCommandService.hpp"

#include "json/GlazeCompat.hpp"

#include <memory>
#include <string>

class IChatSession;

// Transport-role interface for in-process group message dispatch.
//
// The command/query surface (MessageCommandResult Xxx(request) + BuildGroupListJson) lives on the
// base IGroupMessageCommandService and is consumed independently by the internal gRPC service
// (server-to-server). This interface IS-A command service and adds ONLY the session-bound transport
// handlers (HandleXxx) used by the in-process registrar dispatch. Keeping the two roles split lets
// each consumer depend on the narrowest surface it needs (ISP).
class IGroupMessageService : public IGroupMessageCommandService
{
public:
    virtual ~IGroupMessageService() = default;

    virtual void
    HandleCreateGroup(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGetGroupList(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleInviteGroupMember(const std::shared_ptr<IChatSession>& session,
                                         short msg_id,
                                         const std::string& msg_data) = 0;
    virtual void
    HandleApplyJoinGroup(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleReviewGroupApply(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGroupChatMessage(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGroupHistory(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleEditGroupMessage(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleRevokeGroupMessage(const std::shared_ptr<IChatSession>& session,
                                          short msg_id,
                                          const std::string& msg_data) = 0;
    virtual void HandleForwardGroupMessage(const std::shared_ptr<IChatSession>& session,
                                           short msg_id,
                                           const std::string& msg_data) = 0;
    virtual void
    HandleGroupReadAck(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void HandleUpdateGroupAnnouncement(const std::shared_ptr<IChatSession>& session,
                                               short msg_id,
                                               const std::string& msg_data) = 0;
    virtual void
    HandleUpdateGroupIcon(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleSetGroupAdmin(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleMuteGroupMember(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleKickGroupMember(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleQuitGroup(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleDissolveGroup(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data) = 0;
};
