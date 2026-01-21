#pragma once
#include <functional>
#include <memory>
#include <string>
#include <json/json.h>

// [关键修改] 引用 GateServer 的 const.h 来获取 ErrorCodes 定义
// 这样就不会报 "enum类型重定义" 错误了
#include "../GateServer/const.h"

enum MessageId {
    MSG_CHAT_LOGIN = 1005, 
    MSG_CHAT_LOGIN_RSP = 1006,
};

// [关键修改] 删除这里的 ErrorCodes 定义，因为上面include已经有了
// 删掉下面这段：
// enum ErrorCodes {
//     Success = 0,
//     ...
// };

#define MAX_LENGTH  1024 * 2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000