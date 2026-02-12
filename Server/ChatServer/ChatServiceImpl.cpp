#include "ChatServiceImpl.h"
#include "UserMgr.h"
#include "CSession.h"
#include <iostream>
#include <json/json.h>
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "ConfigMgr.h"
#include "const.h"

ChatServiceImpl::ChatServiceImpl() {}

Status ChatServiceImpl::NotifyAddFriend(ServerContext* context, const AddFriendReq* request,
    AddFriendRsp* reply) {
    int touid = request->touid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, reply]() {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
        });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_ADD_FRIEND_REQ);

    return Status::OK;
}

bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& user_info) {
    // 1. Read from Redis
    std::string info_str = "";
    bool b_redis = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_redis) {
        Json::Reader reader;
        Json::Value root;
        reader.parse(info_str, root);
        user_info->uid = root["uid"].asInt();
        user_info->name = root["name"].asString();
        user_info->nick = root["nick"].asString();
        user_info->icon = root["icon"].asString();
        user_info->sex = root["sex"].asInt();
        return true;
    }

    // 2. Read from MySQL
    std::shared_ptr<UserInfo> user_info_mysql = nullptr;
    user_info_mysql = MysqlMgr::GetInstance()->GetUser(uid);
    if (user_info_mysql == nullptr) {
        return false;
    }

    user_info = user_info_mysql;

    // 3. Write to Redis
    Json::Value redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["name"] = user_info->name;
    redis_root["nick"] = user_info->nick;
    redis_root["icon"] = user_info->icon;
    redis_root["sex"] = user_info->sex;
    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
    return true;
}

Status ChatServiceImpl::NotifyAuthFriend(ServerContext* context,
    const AuthFriendReq* request, AuthFriendRsp* response) {
    //查找用户是否在本服务器
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::GetInstance()->GetSession(touid);

    Defer defer([request, response]() {
        response->set_error(ErrorCodes::Success);
        response->set_fromuid(request->fromuid());
        response->set_touid(request->touid());
    });

    //用户不在内存中则直接返回
    if (session == nullptr) {
        return Status::OK;
    }

    //在内存中则直接发送通知对方
    Json::Value  rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, fromuid, user_info);
    if (b_info) {
        rtvalue["name"] = user_info->name;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }

    std::string return_str = rtvalue.toStyledString();

    session->Send(return_str, ID_NOTIFY_AUTH_FRIEND_REQ);
    return Status::OK;
}

Status ChatServiceImpl::NotifyTextChatMsg(::grpc::ServerContext* context,
    const TextChatMsgReq* request, TextChatMsgRsp* response) {
    // Day 29: 聊天消息通知
    return Status::OK;
}