#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include <iostream>

ChatServiceImpl::ChatServiceImpl() {}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
    AddFriendRsp* reply) {
    // Day 28: 查找本服用户并推送请求
    return Status::OK;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context,
    const AuthFriendReq* request, AuthFriendRsp* response) {
    // Day 29: 好友认证通知
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
    const TextChatMsgReq* request, TextChatMsgRsp* response) {
    // Day 29: 聊天消息通知
    return Status::OK;
}