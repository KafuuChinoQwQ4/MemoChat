#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Singleton.h"
#include <assert.h>
#include <queue>
#include <pqxx/pqxx>
#include <functional>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

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
    ProfileUpFailed = 1012,
    MediaUploadFailed = 1013,
    ClientVersionTooLow = 1014,
    CallBusy = 1015,
    CallNotFound = 1016,
    CallStateInvalid = 1017,
    CallPermissionDenied = 1018,
    CallTargetOffline = 1019,
    ChatTicketInvalid = 1095,
    ChatTicketExpired = 1096,
    ChatServerMismatch = 1097,
};

class Defer {
public:
    Defer(std::function<void()> func) : func_(func) {}
    ~Defer() { func_(); }

private:
    std::function<void()> func_;
};

#define CODEPREFIX "code_"
#define USERIPPREFIX "uip_"
#define USERTOKENPREFIX "utoken_"
#define SERVER_ONLINE_USERS_PREFIX "server_online_users_"
#define CALL_SESSION_PREFIX "call:session:"
#define CALL_ACTIVE_PREFIX "call:user_active:"
#define CALL_RINGING_PREFIX "call:ringing:"
