#pragma once
#include <functional>

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
    PasswdInvalid = 1009,
    TokenInvalid = 1010,
    UidInvalid = 1011,
    GroupPermissionDenied = 1012,
    GroupNotFound = 1013,
    GroupMemberNotFound = 1014,
    GroupMemberLimit = 1015,
    GroupMuted = 1016,
    GroupApplyNotFound = 1017,
};

class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() { func_(); }

private:
    std::function<void()> func_;
};

#define MAX_LENGTH 1024 * 2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE 10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
    MSG_CHAT_LOGIN = 1005,
    MSG_CHAT_LOGIN_RSP = 1006,
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008,
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1013,
    ID_AUTH_FRIEND_RSP = 1014,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,
    ID_TEXT_CHAT_MSG_REQ = 1017,
    ID_TEXT_CHAT_MSG_RSP = 1018,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
    ID_NOTIFY_OFF_LINE_REQ = 1021,
    ID_HEART_BEAT_REQ = 1023,
    ID_HEARTBEAT_RSP = 1024,
    ID_CREATE_GROUP_REQ = 1031,
    ID_CREATE_GROUP_RSP = 1032,
    ID_GET_GROUP_LIST_REQ = 1033,
    ID_GET_GROUP_LIST_RSP = 1034,
    ID_INVITE_GROUP_MEMBER_REQ = 1035,
    ID_INVITE_GROUP_MEMBER_RSP = 1036,
    ID_NOTIFY_GROUP_INVITE_REQ = 1037,
    ID_APPLY_JOIN_GROUP_REQ = 1038,
    ID_APPLY_JOIN_GROUP_RSP = 1039,
    ID_NOTIFY_GROUP_APPLY_REQ = 1040,
    ID_REVIEW_GROUP_APPLY_REQ = 1041,
    ID_REVIEW_GROUP_APPLY_RSP = 1042,
    ID_NOTIFY_GROUP_MEMBER_CHANGED_REQ = 1043,
    ID_GROUP_CHAT_MSG_REQ = 1044,
    ID_GROUP_CHAT_MSG_RSP = 1045,
    ID_NOTIFY_GROUP_CHAT_MSG_REQ = 1046,
    ID_GROUP_HISTORY_REQ = 1047,
    ID_GROUP_HISTORY_RSP = 1048,
    ID_UPDATE_GROUP_ANNOUNCEMENT_REQ = 1049,
    ID_UPDATE_GROUP_ANNOUNCEMENT_RSP = 1050,
    ID_MUTE_GROUP_MEMBER_REQ = 1051,
    ID_MUTE_GROUP_MEMBER_RSP = 1052,
    ID_SET_GROUP_ADMIN_REQ = 1053,
    ID_SET_GROUP_ADMIN_RSP = 1054,
    ID_KICK_GROUP_MEMBER_REQ = 1055,
    ID_KICK_GROUP_MEMBER_RSP = 1056,
    ID_QUIT_GROUP_REQ = 1057,
    ID_QUIT_GROUP_RSP = 1058,
};

#define USERIPPREFIX "uip_"
#define USERTOKENPREFIX "utoken_"
#define IPCOUNTPREFIX "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT "logincount"
#define NAME_INFO "nameinfo_"
#define LOCK_PREFIX "lock_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_COUNT "lockcount"

#define LOCK_TIME_OUT 10
#define ACQUIRE_TIME_OUT 5
