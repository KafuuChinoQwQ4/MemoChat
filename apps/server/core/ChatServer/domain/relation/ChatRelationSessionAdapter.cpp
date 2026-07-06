#include "ChatRelationSessionAdapter.hpp"

#include "ChatRelationCommandDtos.hpp"
#include "IChatSession.hpp"
#include "const.hpp"
#include "json/GlazeCompat.hpp"

namespace
{
namespace ChatRelationDtos = memochat::chat::relation;

std::string CompactJson(const memochat::json::JsonValue& value)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, value);
}

RelationCommandRequest
BuildRelationCommandRequest(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data)
{
    RelationCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    if (session)
    {
        request.session_uid = session->userId();
        request.session_id = session->sessionId();
    }
    return request;
}

void SendRelationCommandResult(const std::shared_ptr<IChatSession>& session, const RelationCommandResult& result)
{
    if (!session)
    {
        return;
    }
    session->send(result.payload_json, result.response_msg_id);
}

int ResolveDialogListUid(const std::shared_ptr<IChatSession>& session, const std::string& msg_data)
{
    const int session_uid = session ? session->userId() : 0;
    if (session_uid > 0)
    {
        return session_uid;
    }

    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    reader.parse(msg_data, root);
    return ChatRelationDtos::ChatDialogListRequestFromJsonValue(root).uid;
}

void SendDialogListQueryResult(const std::shared_ptr<IChatSession>& session,
                               IRelationService& relation_service,
                               const std::string& msg_data)
{
    if (!session)
    {
        return;
    }

    const int uid = ResolveDialogListUid(session, msg_data);
    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload["error"] = uid > 0 ? ErrorCodes::Success : ErrorCodes::Error_Json;
    payload["uid"] = uid;
    if (uid > 0)
    {
        relation_service.BuildDialogListJson(uid, payload);
    }
    session->send(CompactJson(payload), ID_GET_DIALOG_LIST_RSP);
}
} // namespace

ChatRelationSessionAdapter::ChatRelationSessionAdapter(IRelationService* relation_service)
    : _relation_service(relation_service)
{
}

void ChatRelationSessionAdapter::HandleSearchUser(const std::shared_ptr<IChatSession>& session,
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

void ChatRelationSessionAdapter::HandleAddFriendApply(const std::shared_ptr<IChatSession>& session,
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

void ChatRelationSessionAdapter::HandleAuthFriendApply(const std::shared_ptr<IChatSession>& session,
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

void ChatRelationSessionAdapter::HandleDeleteFriend(const std::shared_ptr<IChatSession>& session,
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

void ChatRelationSessionAdapter::HandleGetDialogList(const std::shared_ptr<IChatSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    if (!_relation_service)
    {
        return;
    }
    (void) msg_id;
    SendDialogListQueryResult(session, *_relation_service, msg_data);
}

void ChatRelationSessionAdapter::HandleSyncDraft(const std::shared_ptr<IChatSession>& session,
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

void ChatRelationSessionAdapter::HandlePinDialog(const std::shared_ptr<IChatSession>& session,
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
