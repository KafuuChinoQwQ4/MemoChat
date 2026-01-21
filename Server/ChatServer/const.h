#pragma once
#include <functional>
#include <memory>
#include <string>
#include <json/json.h>

// 消息 ID 定义
enum MessageId {
    MSG_CHAT_LOGIN = 1005, // 登录聊天服务器
    MSG_CHAT_LOGIN_RSP = 1006,
};

// 错误码 (复用 GateServer)
enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,
    RPCFailed = 1002,
    VarifyExpired = 1003,
    VarifyCodeErr = 1004,
    UserExist = 1005,
    PasswdErr = 1006,
    EmailNotMatch = 1007,
    PasswdUpFailed = 1008,
    UserNotExist = 1009,
    TokenInvalid = 1010
};

#define MAX_LENGTH  1024 * 2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000