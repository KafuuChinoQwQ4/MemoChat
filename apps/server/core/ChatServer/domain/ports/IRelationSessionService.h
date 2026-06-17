#pragma once

#include <memory>
#include <string>

class CSession;

class IRelationSessionService
{
public:
    virtual ~IRelationSessionService() = default;

    virtual void
    HandleSearchUser(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleAddFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleAuthFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleDeleteFriend(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleGetDialogList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandleSyncDraft(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
    virtual void
    HandlePinDialog(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) = 0;
};
