#include "ChatRelationSessionAdapter.hpp"

#include "CSession.hpp"

namespace
{
RelationCommandRequest
BuildRelationCommandRequest(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data)
{
    RelationCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    if (session)
    {
        request.session_uid = session->GetUserId();
        request.session_id = session->GetSessionId();
    }
    return request;
}

void SendRelationCommandResult(const std::shared_ptr<CSession>& session, const RelationCommandResult& result)
{
    if (!session)
    {
        return;
    }
    session->Send(result.payload_json, result.response_msg_id);
}
} // namespace

ChatRelationSessionAdapter::ChatRelationSessionAdapter(IRelationService* relation_service)
    : _relation_service(relation_service)
{
}

void ChatRelationSessionAdapter::HandleSearchUser(const std::shared_ptr<CSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(session,
                              _relation_service->SearchUser(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandleAddFriendApply(const std::shared_ptr<CSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(
        session,
        _relation_service->AddFriendApply(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandleAuthFriendApply(const std::shared_ptr<CSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(
        session,
        _relation_service->AuthFriendApply(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandleDeleteFriend(const std::shared_ptr<CSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(session,
                              _relation_service->DeleteFriend(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandleGetDialogList(const std::shared_ptr<CSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(session,
                              _relation_service->GetDialogList(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandleSyncDraft(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(session,
                              _relation_service->SyncDraft(BuildRelationCommandRequest(session, msg_id, msg_data)));
}

void ChatRelationSessionAdapter::HandlePinDialog(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    SendRelationCommandResult(session,
                              _relation_service->PinDialog(BuildRelationCommandRequest(session, msg_id, msg_data)));
}
