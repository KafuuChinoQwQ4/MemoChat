#include "RelationGrpcServiceAdapter.h"

#include "transport/CSession.h"

#include <utility>

RelationGrpcServiceAdapter::RelationGrpcServiceAdapter(const std::string& endpoint, std::chrono::milliseconds timeout)
    : _client(endpoint, timeout)
{
}

RelationGrpcServiceAdapter::RelationGrpcServiceAdapter(std::shared_ptr<grpc::Channel> channel,
                                                       std::chrono::milliseconds timeout)
    : _client(std::move(channel), timeout)
{
}

void RelationGrpcServiceAdapter::AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out)
{
    _client.AppendRelationBootstrapJson(uid, out);
}

void RelationGrpcServiceAdapter::BuildDialogListJson(int uid, memochat::json::JsonValue& out)
{
    _client.BuildDialogListJson(uid, out);
}

RelationCommandResult RelationGrpcServiceAdapter::SearchUser(const RelationCommandRequest& request)
{
    return _client.SearchUser(request);
}

RelationCommandResult RelationGrpcServiceAdapter::AddFriendApply(const RelationCommandRequest& request)
{
    return _client.AddFriendApply(request);
}

RelationCommandResult RelationGrpcServiceAdapter::AuthFriendApply(const RelationCommandRequest& request)
{
    return _client.AuthFriendApply(request);
}

RelationCommandResult RelationGrpcServiceAdapter::DeleteFriend(const RelationCommandRequest& request)
{
    return _client.DeleteFriend(request);
}

RelationCommandResult RelationGrpcServiceAdapter::GetDialogList(const RelationCommandRequest& request)
{
    return _client.GetDialogList(request);
}

RelationCommandResult RelationGrpcServiceAdapter::SyncDraft(const RelationCommandRequest& request)
{
    return _client.SyncDraft(request);
}

RelationCommandResult RelationGrpcServiceAdapter::PinDialog(const RelationCommandRequest& request)
{
    return _client.PinDialog(request);
}

void RelationGrpcServiceAdapter::HandleSearchUser(const std::shared_ptr<CSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    SendSessionCommandResult(session, SearchUser(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandleAddFriendApply(const std::shared_ptr<CSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    SendSessionCommandResult(session, AddFriendApply(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandleAuthFriendApply(const std::shared_ptr<CSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    SendSessionCommandResult(session, AuthFriendApply(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandleDeleteFriend(const std::shared_ptr<CSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    SendSessionCommandResult(session, DeleteFriend(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandleGetDialogList(const std::shared_ptr<CSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    SendSessionCommandResult(session, GetDialogList(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandleSyncDraft(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendSessionCommandResult(session, SyncDraft(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void RelationGrpcServiceAdapter::HandlePinDialog(const std::shared_ptr<CSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendSessionCommandResult(session, PinDialog(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

RelationCommandRequest RelationGrpcServiceAdapter::BuildSessionCommandRequest(const std::shared_ptr<CSession>& session,
                                                                              short msg_id,
                                                                              const std::string& msg_data) const
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

void RelationGrpcServiceAdapter::SendSessionCommandResult(const std::shared_ptr<CSession>& session,
                                                          const RelationCommandResult& result) const
{
    if (!session)
    {
        return;
    }
    session->Send(result.payload_json, result.response_msg_id);
}
