#pragma once

#include "ports/IRelationService.h"
#include "ports/IRelationSessionService.h"

class ChatRelationSessionAdapter final : public IRelationSessionService
{
public:
    explicit ChatRelationSessionAdapter(IRelationService* relation_service);

    void HandleSearchUser(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleAddFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleAuthFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleDeleteFriend(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleGetDialogList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleSyncDraft(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandlePinDialog(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;

private:
    IRelationService* _relation_service = nullptr;
};
