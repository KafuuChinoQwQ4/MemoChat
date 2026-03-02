#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"
#include <mutex>
#include "data.h"
#include "CServer.h"
#include <memory>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::AddFriendReq;
using message::AddFriendRsp;
using message::AuthFriendReq;
using message::AuthFriendRsp;
using message::ChatService;
using message::TextChatMsgReq;
using message::TextChatMsgRsp;
using message::TextChatData;
using message::KickUserReq;
using message::KickUserRsp;
using message::GroupMessageNotifyReq;
using message::GroupMessageNotifyRsp;
using message::GroupEventNotifyReq;
using message::GroupEventNotifyRsp;
using message::GroupMemberBatchReq;
using message::GroupMemberBatchRsp;

class ChatServiceImpl final : public ChatService::Service
{
public:
    ChatServiceImpl();
    Status NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
        AddFriendRsp* reply) override;

    Status NotifyAuthFriend(ServerContext* context,
        const AuthFriendReq* request, AuthFriendRsp* response) override;

    Status NotifyTextChatMsg(::grpc::ServerContext* context,
        const TextChatMsgReq* request, TextChatMsgRsp* response) override;

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);

    Status NotifyKickUser(::grpc::ServerContext* context,
        const KickUserReq* request, KickUserRsp* response) override;

    Status NotifyGroupMessage(::grpc::ServerContext* context,
        const GroupMessageNotifyReq* request, GroupMessageNotifyRsp* response) override;

    Status NotifyGroupEvent(::grpc::ServerContext* context,
        const GroupEventNotifyReq* request, GroupEventNotifyRsp* response) override;

    Status NotifyGroupMemberBatch(::grpc::ServerContext* context,
        const GroupMemberBatchReq* request, GroupMemberBatchRsp* response) override;

    void RegisterServer(std::shared_ptr<CServer> pServer);

private:
    std::shared_ptr<CServer> _p_server;
};
