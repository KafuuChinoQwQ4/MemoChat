#include "MessageGrpcServiceAdapter.hpp"

#include "ChatRuntime.hpp"
#include "const.hpp"
#include "IChatSession.hpp"
#include "json/GlazeCompat.hpp"

#include <utility>

namespace
{
std::string TraceIdFromPayload(const std::string& payload_json)
{
    memochat::json::JsonValue root(memochat::json::object_t{});
    if (!memochat::json::reader_parse(payload_json, root) || !root.isObject())
    {
        return "";
    }
    return root.get("trace_id", "").asString();
}
} // namespace

MessageGrpcServiceAdapter::MessageGrpcServiceAdapter(const std::string& endpoint, std::chrono::milliseconds timeout)
    : _client(endpoint, timeout)
{
}

MessageGrpcServiceAdapter::MessageGrpcServiceAdapter(std::shared_ptr<grpc::Channel> channel,
                                                     std::chrono::milliseconds timeout)
    : _client(std::move(channel), timeout)
{
}

MessageCommandResult MessageGrpcServiceAdapter::TextChatMessage(const MessageCommandRequest& request)
{
    return _client.TextChatMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::ForwardPrivateMessage(const MessageCommandRequest& request)
{
    return _client.ForwardPrivateMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::PrivateReadAck(const MessageCommandRequest& request)
{
    return _client.PrivateReadAck(request);
}

MessageCommandResult MessageGrpcServiceAdapter::EditPrivateMessage(const MessageCommandRequest& request)
{
    return _client.EditPrivateMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::RevokePrivateMessage(const MessageCommandRequest& request)
{
    return _client.RevokePrivateMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::PrivateHistory(const MessageCommandRequest& request)
{
    return _client.PrivateHistory(request);
}

MessageCommandResult MessageGrpcServiceAdapter::CreateGroup(const MessageCommandRequest& request)
{
    return _client.CreateGroup(request);
}

MessageCommandResult MessageGrpcServiceAdapter::GetGroupList(const MessageCommandRequest& request)
{
    return _client.GetGroupList(request);
}

MessageCommandResult MessageGrpcServiceAdapter::InviteGroupMember(const MessageCommandRequest& request)
{
    return _client.InviteGroupMember(request);
}

MessageCommandResult MessageGrpcServiceAdapter::ApplyJoinGroup(const MessageCommandRequest& request)
{
    return _client.ApplyJoinGroup(request);
}

MessageCommandResult MessageGrpcServiceAdapter::ReviewGroupApply(const MessageCommandRequest& request)
{
    return _client.ReviewGroupApply(request);
}

MessageCommandResult MessageGrpcServiceAdapter::GroupChatMessage(const MessageCommandRequest& request)
{
    return _client.GroupChatMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::GroupHistory(const MessageCommandRequest& request)
{
    return _client.GroupHistory(request);
}

MessageCommandResult MessageGrpcServiceAdapter::GroupReadAck(const MessageCommandRequest& request)
{
    return _client.GroupReadAck(request);
}

MessageCommandResult MessageGrpcServiceAdapter::EditGroupMessage(const MessageCommandRequest& request)
{
    return _client.EditGroupMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::RevokeGroupMessage(const MessageCommandRequest& request)
{
    return _client.RevokeGroupMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::ForwardGroupMessage(const MessageCommandRequest& request)
{
    return _client.ForwardGroupMessage(request);
}

MessageCommandResult MessageGrpcServiceAdapter::UpdateGroupAnnouncement(const MessageCommandRequest& request)
{
    return _client.UpdateGroupAnnouncement(request);
}

MessageCommandResult MessageGrpcServiceAdapter::UpdateGroupIcon(const MessageCommandRequest& request)
{
    return _client.UpdateGroupIcon(request);
}

MessageCommandResult MessageGrpcServiceAdapter::SetGroupAdmin(const MessageCommandRequest& request)
{
    return _client.SetGroupAdmin(request);
}

MessageCommandResult MessageGrpcServiceAdapter::MuteGroupMember(const MessageCommandRequest& request)
{
    return _client.MuteGroupMember(request);
}

MessageCommandResult MessageGrpcServiceAdapter::KickGroupMember(const MessageCommandRequest& request)
{
    return _client.KickGroupMember(request);
}

MessageCommandResult MessageGrpcServiceAdapter::QuitGroup(const MessageCommandRequest& request)
{
    return _client.QuitGroup(request);
}

MessageCommandResult MessageGrpcServiceAdapter::DissolveGroup(const MessageCommandRequest& request)
{
    return _client.DissolveGroup(request);
}

void MessageGrpcServiceAdapter::HandleTextChatMessage(const std::shared_ptr<IChatSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    SendSessionCommandResult(session, TextChatMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleForwardPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                                            short msg_id,
                                                            const std::string& msg_data)
{
    SendSessionCommandResult(session, ForwardPrivateMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandlePrivateReadAck(const std::shared_ptr<IChatSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    SendSessionCommandResult(session, PrivateReadAck(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleEditPrivateMessage(const std::shared_ptr<IChatSession>& session,
                                                         short msg_id,
                                                         const std::string& msg_data)
{
    SendSessionCommandResult(session, EditPrivateMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleRevokePrivateMessage(const std::shared_ptr<IChatSession>& session,
                                                           short msg_id,
                                                           const std::string& msg_data)
{
    SendSessionCommandResult(session, RevokePrivateMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandlePrivateHistory(const std::shared_ptr<IChatSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    SendSessionCommandResult(session, PrivateHistory(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
{
    _client.BuildGroupListJson(uid, out);
}

void MessageGrpcServiceAdapter::HandleCreateGroup(const std::shared_ptr<IChatSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    SendSessionCommandResult(session, CreateGroup(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleGetGroupList(const std::shared_ptr<IChatSession>& session,
                                                   short msg_id,
                                                   const std::string& msg_data)
{
    SendSessionCommandResult(session, GetGroupList(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleInviteGroupMember(const std::shared_ptr<IChatSession>& session,
                                                        short msg_id,
                                                        const std::string& msg_data)
{
    SendSessionCommandResult(session, InviteGroupMember(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleApplyJoinGroup(const std::shared_ptr<IChatSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data)
{
    SendSessionCommandResult(session, ApplyJoinGroup(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleReviewGroupApply(const std::shared_ptr<IChatSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    SendSessionCommandResult(session, ReviewGroupApply(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleGroupChatMessage(const std::shared_ptr<IChatSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    SendSessionCommandResult(session, GroupChatMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleGroupHistory(const std::shared_ptr<IChatSession>& session,
                                                   short msg_id,
                                                   const std::string& msg_data)
{
    SendSessionCommandResult(session, GroupHistory(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleEditGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                       short msg_id,
                                                       const std::string& msg_data)
{
    SendSessionCommandResult(session, EditGroupMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleRevokeGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                         short msg_id,
                                                         const std::string& msg_data)
{
    SendSessionCommandResult(session, RevokeGroupMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleForwardGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                          short msg_id,
                                                          const std::string& msg_data)
{
    SendSessionCommandResult(session, ForwardGroupMessage(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleGroupReadAck(const std::shared_ptr<IChatSession>& session,
                                                   short msg_id,
                                                   const std::string& msg_data)
{
    SendSessionCommandResult(session, GroupReadAck(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleUpdateGroupAnnouncement(const std::shared_ptr<IChatSession>& session,
                                                              short msg_id,
                                                              const std::string& msg_data)
{
    SendSessionCommandResult(session, UpdateGroupAnnouncement(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleUpdateGroupIcon(const std::shared_ptr<IChatSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    SendSessionCommandResult(session, UpdateGroupIcon(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleSetGroupAdmin(const std::shared_ptr<IChatSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    SendSessionCommandResult(session, SetGroupAdmin(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleMuteGroupMember(const std::shared_ptr<IChatSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    SendSessionCommandResult(session, MuteGroupMember(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleKickGroupMember(const std::shared_ptr<IChatSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data)
{
    SendSessionCommandResult(session, KickGroupMember(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleQuitGroup(const std::shared_ptr<IChatSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendSessionCommandResult(session, QuitGroup(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

void MessageGrpcServiceAdapter::HandleDissolveGroup(const std::shared_ptr<IChatSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    SendSessionCommandResult(session, DissolveGroup(BuildSessionCommandRequest(session, msg_id, msg_data)));
}

MessageCommandRequest
MessageGrpcServiceAdapter::BuildSessionCommandRequest(const std::shared_ptr<IChatSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data) const
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    request.server_name = memochat::chatruntime::SelfServerName();
    request.trace_id = TraceIdFromPayload(msg_data);
    if (session)
    {
        request.session_uid = session->userId();
        request.session_id = session->sessionId();
    }
    return request;
}

void MessageGrpcServiceAdapter::SendSessionCommandResult(const std::shared_ptr<IChatSession>& session,
                                                         const MessageCommandResult& result) const
{
    if (!session || result.response_msg_id == 0)
    {
        return;
    }
    session->send(result.payload_json, result.response_msg_id);
}
