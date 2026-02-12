#pragma once
#include <functional>
#include <memory>
#include <string>
#include <json/json.h>
#include <functional>

// [关键修改] 引用 GateServer 的 const.h 来获取 ErrorCodes 定义
// 这样就不会报 "enum类型重定义" 错误了
#include "../GateServer/const.h"

enum MessageId {
    MSG_CHAT_LOGIN = 1005, 
    MSG_CHAT_LOGIN_RSP = 1006,
    
    // [新增] 兼容客户端的定义
    ID_CHAT_LOGIN = 1005, 
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008,
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1012,
    ID_AUTH_FRIEND_RSP = 1013,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1014,
    ID_TEXT_CHAT_MSG_REQ = 1015,
    ID_TEXT_CHAT_MSG_RSP = 1016,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1017,
};

const std::string USER_BASE_INFO = "user_base_info_";

#define MAX_LENGTH  1024 * 2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000